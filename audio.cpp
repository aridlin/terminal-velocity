#include "audio.hpp"
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_FLAC
#define MINIAUDIO_IMPLEMENTATION
#include "vendor/miniaudio.h"
#include <algorithm>
#include <array>
#include <cmath>

struct GameAudio::Impl {
  ma_engine engine{};
  ma_context context{};
  bool context_initialized = false;
  bool initialized = false;
  std::string failure;
  struct Clip {
    ma_sound sound{};
    bool loaded = false;
  };
  std::array<Clip, 2> music;
  std::array<std::array<Clip, 2>, size_t(AudioCue::Count)> effects;
  std::array<unsigned, size_t(AudioCue::Count)> next{};
  std::array<double, 2> quiet_time{};
  int current_track = -1;
  AudioScene current_scene = AudioScene::Menu;
  bool was_muted = false, was_focused = true;
  size_t clips = 0;
  ~Impl() {
    if (!initialized) {
      if (context_initialized)
        ma_context_uninit(&context);
      return;
    }
    for (auto &track : music)
      if (track.loaded)
        ma_sound_uninit(&track.sound);
    for (auto &voices : effects)
      for (auto &clip : voices)
        if (clip.loaded)
          ma_sound_uninit(&clip.sound);
    ma_engine_uninit(&engine);
    if (context_initialized)
      ma_context_uninit(&context);
  }
  bool load(Clip &clip, const std::filesystem::path &path, ma_uint32 flags) {
#ifdef _WIN32
    auto result = ma_sound_init_from_file_w(&engine, path.c_str(), flags,
                                            nullptr, nullptr, &clip.sound);
#else
    auto result = ma_sound_init_from_file(&engine, path.c_str(), flags, nullptr,
                                          nullptr, &clip.sound);
#endif
    clip.loaded = result == MA_SUCCESS;
    if (clip.loaded)
      ++clips;
    else if (failure.empty())
      failure = "Brak plików dźwięku. Przywróć folder assets/audio.";
    return clip.loaded;
  }
};
GameAudio::GameAudio() = default;
GameAudio::~GameAudio() = default;
void GameAudio::init(const std::filesystem::path &directory,
                     bool initially_muted, bool offline) {
  directory_ = directory;
  muted = initially_muted;
  offline_ = offline;
  if (!muted)
    ensure_ready();
}
bool GameAudio::ensure_ready() {
  if (attempted_)
    return ready();
  attempted_ = true;
  impl_ = std::make_unique<Impl>();
  auto config = ma_engine_config_init();
  config.channels = 2;
  config.sampleRate = 48000;
  config.noDevice = offline_ ? MA_TRUE : MA_FALSE;
  if (!offline_) {
#ifdef _WIN32
    ma_backend backends[] = {ma_backend_wasapi, ma_backend_dsound,
                             ma_backend_winmm};
#else
    ma_backend backends[] = {ma_backend_pulseaudio, ma_backend_alsa,
                             ma_backend_jack};
#endif
    if (ma_context_init(backends, sizeof(backends) / sizeof(backends[0]),
                        nullptr, &impl_->context) != MA_SUCCESS) {
      impl_->failure =
          "Brak wyjścia audio. Sprawdź urządzenie i uruchom grę ponownie.";
      return false;
    }
    impl_->context_initialized = true;
    config.pContext = &impl_->context;
  }
  // Playback only: no microphone/capture device is requested.
  if (ma_engine_init(&config, &impl_->engine) != MA_SUCCESS) {
    impl_->failure =
        "Brak wyjścia audio. Sprawdź urządzenie i uruchom grę ponownie.";
    return false;
  }
  impl_->initialized = true;
  ma_engine_set_volume(&impl_->engine, .60f);
  auto flags = MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
  const char *tracks[] = {"dreams.mp3", "ascent.mp3"};
  for (size_t i = 0; i < 2; ++i) {
    if (impl_->load(
            impl_->music[i], directory_ / tracks[i],
            flags | (offline_ ? MA_SOUND_FLAG_DECODE : MA_SOUND_FLAG_STREAM))) {
      ma_sound_set_looping(&impl_->music[i].sound, MA_TRUE);
      ma_sound_set_volume(&impl_->music[i].sound, .55f);
      ma_sound_set_fade_in_milliseconds(&impl_->music[i].sound, 0, 0, 1);
    }
  }
  const char *names[] = {"jump",       "dash",   "pickup", "death",
                         "checkpoint", "land",   "step1",  "step2",
                         "refill",     "spring", "menu",   "win"};
  const float volumes[] = {.45f, .4f,  .35f, .4f, .4f,  .3f,
                           .17f, .17f, .35f, .4f, .22f, .45f};
  for (size_t i = 0; i < size_t(AudioCue::Count); ++i)
    for (auto &voice : impl_->effects[i])
      if (impl_->load(voice, directory_ / (std::string(names[i]) + ".wav"),
                      flags | MA_SOUND_FLAG_DECODE))
        ma_sound_set_volume(&voice.sound, volumes[i]);
  return true;
}
void GameAudio::play(AudioCue cue) {
  if (muted || !ensure_ready())
    return;
  size_t i = size_t(cue);
  if (i >= impl_->effects.size())
    return;
  auto &voice = impl_->effects[i][impl_->next[i]++ % 2];
  if (!voice.loaded)
    return;
  ma_sound_seek_to_pcm_frame(&voice.sound, 0);
  ma_sound_start(&voice.sound);
}
void GameAudio::update(AudioScene scene, int track, double dt, bool focused) {
  if (!impl_ && muted)
    return; // --mute never opens an audio device
  if (!ensure_ready())
    return;
  track = std::clamp(track, 0, 1);
  bool changed = track != impl_->current_track ||
                 scene != impl_->current_scene || muted != impl_->was_muted ||
                 focused != impl_->was_focused;
  // Master mute is immediate and also silences already-playing effects.
  ma_engine_set_volume(&impl_->engine, muted || !focused ? 0.f : .60f);
  for (size_t i = 0; i < 2; ++i) {
    auto &music = impl_->music[i];
    if (!music.loaded)
      continue;
    bool selected = int(i) == track && !muted && focused;
    if (selected) {
      impl_->quiet_time[i] = 0;
      if (!ma_sound_is_playing(&music.sound))
        ma_sound_start(&music.sound);
    } else {
      impl_->quiet_time[i] += std::clamp(dt, 0., .1);
      if (impl_->quiet_time[i] > .9)
        ma_sound_stop(&music.sound);
    }
    if (changed) {
      float gain = selected ? (scene == AudioScene::Paused ? .25f
                               : scene == AudioScene::Menu ? .65f
                                                           : 1.f)
                            : 0.f;
      ma_sound_set_fade_in_milliseconds(&music.sound, -1, gain, 800);
    }
  }
  impl_->current_track = track;
  impl_->current_scene = scene;
  impl_->was_muted = muted;
  impl_->was_focused = focused;
}
bool GameAudio::ready() const { return impl_ && impl_->initialized; }
std::string GameAudio::error() const { return impl_ ? impl_->failure : ""; }
size_t GameAudio::loaded_clips() const { return impl_ ? impl_->clips : 0; }
bool GameAudio::render(float *stereo, size_t frames) {
  return offline_ && ready() &&
         ma_engine_read_pcm_frames(&impl_->engine, stereo, frames, nullptr) ==
             MA_SUCCESS;
}
