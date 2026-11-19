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
    float strength = 1.f;
  };
  std::array<Clip, 4> music;
  std::array<Clip, 6> ambience;
  Clip film_take;
  int take_id=-1;
  bool take_polish=false;
  std::array<Clip, 2> climax;
  bool climax_active=false;
  float climax_gain=-1;
  double climax_quiet=0;
  std::array<double, 6> ambience_quiet{};
  std::array<float, size_t(AudioCue::Count)> effect_levels{};
  std::array<std::array<Clip, 2>, size_t(AudioCue::Count)> effects;
  std::array<unsigned, size_t(AudioCue::Count)> next{};
  std::array<double, 4> quiet_time{};
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
    for(auto &layer:climax)if(layer.loaded)ma_sound_uninit(&layer.sound);
    for (auto &track : ambience)
      if (track.loaded) ma_sound_uninit(&track.sound);
    for (auto &voices : effects)
      for (auto &clip : voices)
        if (clip.loaded)
          ma_sound_uninit(&clip.sound);
    if (film_take.loaded) ma_sound_uninit(&film_take.sound);
    ma_engine_uninit(&engine);
    if (context_initialized)
      ma_context_uninit(&context);
  }
  bool load(Clip &clip, const std::filesystem::path &path, ma_uint32 flags, bool required=true) {
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
    else if (required && failure.empty())
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
#if defined(__EMSCRIPTEN__)
    ma_backend backends[] = {ma_backend_webaudio};
#elif defined(_WIN32)
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
  const char *tracks[] = {"dreams.mp3", "ascent.mp3", "snow.mp3", "crown-sentinel.mp3"};
  for (size_t i = 0; i < impl_->music.size(); ++i) {
    if (impl_->load(
            impl_->music[i], directory_ / tracks[i],
            flags |
#ifdef __EMSCRIPTEN__
            0 // Keep encoded music in memory; Web builds have no streaming worker.
#else
            (offline_ ? MA_SOUND_FLAG_DECODE : MA_SOUND_FLAG_STREAM)
#endif
      )) {
      ma_sound_set_looping(&impl_->music[i].sound, MA_TRUE);
      ma_sound_set_volume(&impl_->music[i].sound, .55f);
      ma_sound_set_fade_in_milliseconds(&impl_->music[i].sound, 0, 0, 1);
    }
  }
  const char *layers[]={"crown-foundation.wav","crown-tension.wav"};
  for(size_t i=0;i<impl_->climax.size();++i)if(impl_->load(impl_->climax[i],directory_/layers[i],flags|MA_SOUND_FLAG_DECODE)) {
    ma_sound_set_looping(&impl_->climax[i].sound,MA_TRUE);
    ma_sound_set_volume(&impl_->climax[i].sound,0);
  }
  const char *environments[] = {"harbor.wav", "works.wav", "streets.wav", "archive.wav", "gardens.wav", "crown.wav"};
  for (size_t i = 0; i < impl_->ambience.size(); ++i)
    if (impl_->load(impl_->ambience[i], directory_ / environments[i], flags | MA_SOUND_FLAG_DECODE)) {
      ma_sound_set_looping(&impl_->ambience[i].sound, MA_TRUE);
      ma_sound_set_volume(&impl_->ambience[i].sound, 0);
    }
  const char *names[] = {"jump",       "dash",   "pickup", "death",
                         "checkpoint", "land",   "step1",  "step2",
                         "refill", "spring", "menu", "win",
    "wood1", "wood2", "metal1", "metal2", "stone1", "stone2", "valve", "voice", "paper", "bounce", "door", "steam", "lift", "cloth", "motor", "rumble", "spark", "talk0", "talk1", "talk2", "talk3", "talk4", "talk5", "talk6", "talk7"};
  const float volumes[] = {.45f, .4f,  .35f, .4f, .4f,  .3f,
                           .17f, .17f, .35f, .4f, .22f, .45f,
    .38f,.38f,.32f,.32f,.4f,.4f,.55f,.3f,.42f,.4f, .32f,.22f,.25f,.2f,.36f,.42f,.25f, .40f,.38f,.36f,.36f,.34f,.38f,.34f,.32f};
  std::copy(std::begin(volumes), std::end(volumes), impl_->effect_levels.begin());
  for (size_t i = 0; i < size_t(AudioCue::Count); ++i)
    for (auto &voice : impl_->effects[i])
      if (impl_->load(voice, directory_ / (std::string(names[i]) + ".wav"),
                      flags | MA_SOUND_FLAG_DECODE))
        ma_sound_set_volume(&voice.sound, volumes[i]);
  return true;
}
void GameAudio::play(AudioCue cue, float strength) {
  if (muted || !ensure_ready())
    return;
  if (environment_ >= 0 && (cue == AudioCue::Step1 || cue == AudioCue::Step2 || cue == AudioCue::Land)) {
    bool second = cue == AudioCue::Step2;
    if (environment_ == 0 || environment_ == 2) cue = second ? AudioCue::Wood2 : AudioCue::Wood1;
    else if (environment_ == 1 || environment_ == 3) cue = second ? AudioCue::Metal2 : AudioCue::Metal1;
    else cue = second ? AudioCue::Stone2 : AudioCue::Stone1;
  }
  size_t i = size_t(cue);
  if (i >= impl_->effects.size())
    return;
  auto &voice = impl_->effects[i][impl_->next[i]++ % 2];
  if (!voice.loaded)
    return;
  voice.strength=std::clamp(strength,0.f,1.f);
  ma_sound_set_volume(&voice.sound, impl_->effect_levels[i] * effects_gain_ * voice.strength);
  ma_sound_seek_to_pcm_frame(&voice.sound, 0);
  ma_sound_start(&voice.sound);
}
void GameAudio::film_voice(int line, bool polish, double seconds, bool playing) {
  if (!impl_ || !ready()) return;
  auto &take=impl_->film_take;
  if (line!=impl_->take_id || polish!=impl_->take_polish) {
    if (take.loaded) {
      ma_sound_uninit(&take.sound);
      take.loaded=false;
      --impl_->clips;
    }
    impl_->take_id=line;
    impl_->take_polish=polish;
    if (line>=0) {
      auto name=std::string(polish?"pl-":"en-")+std::to_string(line)+".mp3";
      impl_->load(take,directory_/"crown-film"/name,
        MA_SOUND_FLAG_NO_SPATIALIZATION|MA_SOUND_FLAG_WAIT_INIT|MA_SOUND_FLAG_DECODE,false);
    }
  }
  if (!take.loaded) return;
  ma_sound_set_volume(&take.sound,.90f*effects_gain_);
  if (!playing || muted) {
    ma_sound_stop(&take.sound);
    return;
  }
  if (ma_sound_is_playing(&take.sound)) return;
  ma_format format;
  ma_uint32 channels, rate;
  ma_uint64 length;
  if (ma_sound_get_data_format(&take.sound,&format,&channels,&rate,nullptr,0)!=MA_SUCCESS ||
      ma_sound_get_length_in_pcm_frames(&take.sound,&length)!=MA_SUCCESS) return;
  const auto frame=ma_uint64(seconds*rate);
  if (frame<length) {
    ma_sound_seek_to_pcm_frame(&take.sound,frame);
    ma_sound_start(&take.sound);
  }
}
void GameAudio::update(AudioScene scene, int track, double dt, bool focused) {
  if (!impl_ && muted)
    return; // --mute never opens an audio device
  if (!ensure_ready())
    return;
  track = std::clamp(track, 0, int(impl_->music.size())-1);
  bool changed = track != impl_->current_track ||
                 scene != impl_->current_scene || muted != impl_->was_muted ||
                 focused != impl_->was_focused;
  bool layered=climax_&&track==1&&!silence_;
  bool entering=layered&&(!impl_->climax_active||(!impl_->was_focused&&focused)||(impl_->was_muted&&!muted));
  if(entering) {
    // One engine clock starts the score and both atmosphere layers together.
    auto start=ma_engine_get_time_in_pcm_frames(&impl_->engine)+ma_engine_get_sample_rate(&impl_->engine)/20;
    auto schedule=[&](Impl::Clip &clip){if(clip.loaded){ma_sound_stop(&clip.sound);ma_sound_seek_to_pcm_frame(&clip.sound,0);ma_sound_set_start_time_in_pcm_frames(&clip.sound,start);ma_sound_start(&clip.sound);}};
    schedule(impl_->music[1]);for(auto &clip:impl_->climax)schedule(clip);
    changed=true;
  }
  // Master mute is immediate and also silences already-playing effects.
  ma_engine_set_volume(&impl_->engine, muted || !focused ? 0.f : .60f);
  for (size_t i = 0; i < impl_->music.size(); ++i) {
    auto &music = impl_->music[i];
    if (!music.loaded)
      continue;
    bool selected = int(i) == track && !muted && focused && !silence_;
    ma_sound_set_volume(&music.sound, (layered?.40f:.48f) * music_gain_);
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
  for (size_t i = 0; i < impl_->ambience.size(); ++i) {
    auto &clip = impl_->ambience[i];
    if (!clip.loaded) continue;
    bool selected = int(i) == environment_ && !muted && focused && !silence_ && scene != AudioScene::Menu;
    float gain = selected ? .55f * ambience_gain_ * (scene == AudioScene::Paused ? .25f : 1.f) : 0.f;
    ma_sound_set_volume(&clip.sound, gain);
    if (selected) {
      impl_->ambience_quiet[i] = 0;
      if (!ma_sound_is_playing(&clip.sound)) ma_sound_start(&clip.sound);
    } else if ((impl_->ambience_quiet[i] += std::clamp(dt, 0., .1)) > .9) ma_sound_stop(&clip.sound);
  }
  float layer_gain=layered?(scene==AudioScene::Paused?.25f:scene==AudioScene::Menu?0.f:1.f):0.f;
  float wanted=layer_gain*climax_intensity_;
  if(entering||std::abs(wanted-impl_->climax_gain)>.001f||changed) {
    for(size_t i=0;i<impl_->climax.size();++i) {
      auto &clip=impl_->climax[i];if(!clip.loaded)continue;
      ma_sound_set_volume(&clip.sound,music_gain_*(i?.32f:.40f));
      float gain=i?wanted*wanted:wanted;
      ma_sound_set_fade_in_milliseconds(&clip.sound,-1,gain,entering?1200:800);
    }
    impl_->climax_gain=wanted;
  }
  // Channel controls apply immediately even while an intensity fade is active.
  for(size_t i=0;i<impl_->climax.size();++i)if(impl_->climax[i].loaded)
    ma_sound_set_volume(&impl_->climax[i].sound,music_gain_*(i?.32f:.40f));
  if(layered)impl_->climax_quiet=0;
  else if((impl_->climax_quiet+=std::clamp(dt,0.,.1))>.9)
    for(auto &clip:impl_->climax)if(clip.loaded)ma_sound_stop(&clip.sound);
  impl_->climax_active=layered;
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

void GameAudio::set_mix(float music, float effects, float ambience) {
  music_gain_ = std::clamp(music, 0.f, 1.f);
  effects_gain_ = std::clamp(effects, 0.f, 1.f);
  ambience_gain_ = std::clamp(ambience, 0.f, 1.f);
  if (impl_) for (size_t i=0;i<impl_->effects.size();++i)
    for (auto &clip:impl_->effects[i]) if (clip.loaded)
      ma_sound_set_volume(&clip.sound, impl_->effect_levels[i] * effects_gain_ * clip.strength);
}
void GameAudio::set_environment(int district, bool silence) {
  environment_ = std::clamp(district, -1, 5);
  if (silence_ != silence && impl_) impl_->current_track = -1;
  silence_ = silence;
}

void GameAudio::set_climax(bool enabled,float intensity) {
  climax_=enabled;climax_intensity_=std::clamp(intensity,0.f,1.f);
}
