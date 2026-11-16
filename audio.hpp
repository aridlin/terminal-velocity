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
  Count
};
enum class AudioScene { Menu, Play, Paused, Win };
// Plays downloaded recordings only. Implementation lives outside game physics.
class GameAudio {
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::filesystem::path directory_;
  bool attempted_ = false, offline_ = false;
  bool ensure_ready();

public:
  bool muted = false;
  GameAudio();
  ~GameAudio();
  GameAudio(const GameAudio &) = delete;
  GameAudio &operator=(const GameAudio &) = delete;
  void init(const std::filesystem::path &directory,
            bool initially_muted = false, bool offline = false);
  void update(AudioScene scene, int track, double dt, bool focused = true);
  void play(AudioCue cue);
  bool ready() const;
  std::string error() const;
  size_t loaded_clips() const;
  bool render(float *stereo,
              size_t frames); // deterministic offline mix for tests
};
