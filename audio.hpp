#pragma once
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

enum class AudioCue {
  Jump,
  Dash,
  Pickup,
  Death,
  Checkpoint,
  Land,
  Step1,
  Step2,
  Refill,
  Spring,
  Menu,
  Win,
  Wood1, Wood2, Metal1, Metal2, Stone1, Stone2,
  Valve, Voice, Paper, Bounce, Door, Steam, Lift, Cloth, Motor, Rumble, Spark,
  Talk0, Talk1, Talk2, Talk3, Talk4, Talk5, Talk6, Talk7,
  Achievement, RareAchievement, Count
};
enum class AudioScene { Menu, Play, Paused, Win };
// Plays downloaded recordings only. Implementation lives outside game physics.
class GameAudio {
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::filesystem::path directory_;
  bool attempted_ = false, offline_ = false;
  bool ensure_ready();
  float music_gain_ = 1, effects_gain_ = 1, ambience_gain_ = 1;
  int environment_ = -1;
  bool silence_ = false;
  bool climax_ = false;
  float climax_intensity_ = 0;

public:
  bool muted = false;
  GameAudio();
  ~GameAudio();
  GameAudio(const GameAudio &) = delete;
  GameAudio &operator=(const GameAudio &) = delete;
  void init(const std::filesystem::path &directory,
            bool initially_muted = false, bool offline = false);
  void update(AudioScene scene, int track, double dt, bool focused = true);
  void play(AudioCue cue, float strength = 1.f);
  void film_voice(int line, bool polish, double seconds, bool playing);
  void set_mix(float music, float effects, float ambience);
  void set_environment(int district, bool silence = false);
  void set_climax(bool enabled, float intensity = 0);
  bool ready() const;
  std::string error() const;
  size_t loaded_clips() const;
  bool render(float *stereo,
              size_t frames); // deterministic offline mix for tests
};
