#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
  std::cout << "PASS " << message << '\n';
}
int main() {
  try {
    check(run_stars(4, 0, 12) == 1 && run_stars(0, 0, 12) == 2 &&
              run_stars(3, 12, 12) == 2 && run_stars(0, 12, 12) == 3,
          "separate completion, clean-run and collection goals");
    for (int chapter = 1; chapter <= 3; ++chapter) {
      auto world = build_chapter(chapter);
      auto run = new_state(world);
      FixedStepper stepper;
      for (int tick = 0; tick < 120 * 100 && !run.won; ++tick) {
        auto &p = run.player;
        Controls c;
        c.move = 1;
        c.jump_held = true;
        int foot = int(std::floor(p.y + kPlayerHeight + .05));
        bool trouble = false;
        for (double dx = 1; dx <= 2.5; dx += .5) {
          int x = int(p.x + kPlayerWidth + dx);
          trouble |= world.solid_at(x, foot - 1) ||
                     world.hazard_at(x, foot - 1) || world.hazard_at(x, 37) ||
                     !world.solid_at(x, foot);
        }
        c.jump_pressed = p.grounded && trouble;
        c.dash_pressed = !p.grounded && p.vy >= 0 &&
                         !world.solid_at(int(p.x + kPlayerWidth + 1), 38) &&
                         p.dash_available;
        stepper.advance(run, world, c, 1. / 120);
      }
      std::cout << "chapter=" << chapter << " x=" << run.player.x
                << " deaths=" << run.deaths << " time=" << run.elapsed << '\n';
      check(run.won && run.deaths == 0,
            "authored chapter completed with normal jump/dash inputs");
      check(course_id(world) == chapter + 2,
            "campaign ghost course identity isolated");
    }
    {
      auto level = build_chapter(1);
      auto state = new_state(level);
      Controls retry;
      retry.restart = true;
      FixedStepper stepper;
      stepper.advance(state, level, retry, 1. / 120);
      check(state.deaths == 1 && run_stars(state.deaths, 0, 12) == 1,
            "checkpoint retries count toward the clean-run challenge");
    }
    auto folder =
        std::filesystem::temp_directory_path() /
        ("tv-progress-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    auto file = folder / "progress.dat";
    ProgressStore a(file), b(file);
    bool best = false;
    check(a.unlocked == 1, "new profile starts at chapter one");
    check(a.record(1, 20, 2, 1, 12, best) && best && a.unlocked == 2,
          "completion saves and unlocks next chapter");
    check(a.record(1, 21, 0, 2, 12, best) && a.records[0].stars == 2,
          "clean-run star earned independently of collecting treasures");
    check(a.record(1, 25, 3, 12, 12, best) && !best,
          "slower replay preserves faster time");
    check(a.records[0].best_time == 20 && a.records[0].best_deaths == 0 &&
              a.records[0].stars == 3,
          "best time, clean run and collection stars preserved independently");
    check(b.record(2, 30, 0, 0, 12, best), "second local profile writer saves");
    ProgressStore loaded(file);
    check(loaded.unlocked == 3 && loaded.records[0].stars == 3 &&
              loaded.records[1].complete,
          "progress persists and merges across local game clients");
    auto g = build_chapter(3);
    auto s = new_state(g);
    s.won = true;
    s.elapsed = 44;
    finish_chapter(s, g, loaded);
    check(s.result_recorded && s.earned_stars == 2 && !s.progress_error,
          "result recorded once and exposes earned stars");
    auto original = loaded.records[2].best_time;
    s.elapsed = 12;
    finish_chapter(s, g, loaded);
    check(loaded.records[2].best_time == original,
          "win screen does not repeatedly rewrite records");
    {
      std::ofstream out(file);
      out << "TVP1 3\ninvalid save";
    }
    ProgressStore corrupt(file);
    check(corrupt.unlocked == 1 && !corrupt.records[0].complete,
          "malformed profile safely ignored");
    auto blocked = folder / "directory.dat";
    std::filesystem::create_directory(blocked);
    ProgressStore read_only(blocked);
    check(!read_only.record(1, 20, 0, 0, 12, best) &&
              read_only.records[0].complete,
          "save failure retains session result without stopping play");
    std::filesystem::remove_all(folder);

    GameAudio sound;
    sound.init("assets/audio", false, true);
    check(sound.ready() && sound.loaded_clips() == 26 && sound.error().empty(),
          "all sourced music and effect voices decode offline");
    sound.update(AudioScene::Play, 0, .02);
    for (int voice = 0; voice < 2; ++voice)
      for (int i = 0; i < int(AudioCue::Count); ++i)
        sound.play(AudioCue(i));
    std::vector<float> frames(48000 * 2);
    check(sound.render(frames.data(), 48000),
          "music and overlapping effects mix from real recordings");
    double peak = 0, energy = 0;
    for (float sample : frames) {
      if (!std::isfinite(sample))
        throw std::runtime_error("non-finite audio sample");
      peak = std::max(peak, std::abs(double(sample)));
      energy += sample * sample;
    }
    std::cout << "audio peak=" << peak
              << " rms=" << std::sqrt(energy / frames.size()) << '\n';
    check(peak < .98 && energy > .001,
          "conservative overlapping audio mix leaves headroom");
    sound.muted = true;
    sound.update(AudioScene::Play, 0, .02);
    check(sound.render(frames.data(), 48000), "muted mix renders");
    check(std::all_of(frames.begin(), frames.end(),
                      [](float x) { return std::abs(x) < 1e-7f; }),
          "mute silences music and already-playing effects");
    sound.muted = false;
    sound.update(AudioScene::Play, 1, .02);
    check(sound.render(frames.data(), 48000), "second music track crossfades");
    sound.update(AudioScene::Paused, 1, .02, false);
    sound.render(frames.data(), 48000);
    check(std::all_of(frames.begin(), frames.end(),
                      [](float x) { return std::abs(x) < 1e-7f; }),
          "focus loss silences audio immediately");
    GameAudio silent;
    silent.init("missing-audio", true);
    check(!silent.ready() && silent.error().empty(),
          "--mute does not open an audio device or load assets");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
