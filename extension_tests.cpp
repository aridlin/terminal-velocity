#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>
void require(bool value, const char *why) {
  if (!value)
    throw std::runtime_error(why);
  std::cout << "PASS " << why << '\n';
}
int main() {
  try {
    auto a = build_procedural(73), b = build_procedural(73),
         c = build_procedural(74);
    bool same = true, different = false;
    for (int y = 0; y < a.height(); ++y)
      for (int x = 0; x < a.width(); ++x) {
        same &= a.solid_at(x, y) == b.solid_at(x, y);
        different |= a.solid_at(x, y) != c.solid_at(x, y);
      }
    require(same && a.sweets == b.sweets && different,
            "seeds repeat exactly and different seeds vary");
    EngineGrid g(80, 46);
    g.spawn_x = 10;
    g.spawn_y = 29 - kPlayerHeight;
    g.goal = {75, 20, 2, 5};
    g.add_solid_rect(0, 40, 80, 6);
    g.platforms.push_back({{8, 30, 6, .5}, 2, 0, 4, 0});
    auto s = new_state(g);
    s.player.y = 30 - kPlayerHeight - .02;
    s.player.vy = 3;
    FixedStepper step;
    step.advance(s, g, {}, .02);
    require(s.player.support == 0 && s.player.grounded,
            "lands on moving one-way platform");
    double offset = s.player.x - g.platforms[0].at(s.elapsed).x;
    for (int i = 0; i < 100; ++i)
      step.advance(s, g, {}, 1. / 120);
    require(s.deaths == 0 && s.player.support == 0 &&
                std::abs((s.player.x - g.platforms[0].at(s.elapsed).x) -
                         offset) < 1e-5,
            "platform carries rider without jitter or drift");
    Controls input;
    input.fast_fall = true;
    step.advance(s, g, input, .05);
    require(s.player.support == -1 && !s.player.grounded,
            "down drops through moving platform");
    s = new_state(g);
    s.player.y = 32;
    s.player.vy = -20;
    step = FixedStepper{};
    input = {};
    input.jump_held = true;
    step.advance(s, g, input, .1);
    require(s.player.y < 31, "upward movement passes through one-way platform");
    g.platforms.clear();
    g.springs.push_back({10, 39.7, 2, .3});
    s = new_state(g);
    s.player.y = 40 - kPlayerHeight;
    s.player.grounded = true;
    step = FixedStepper{};
    step.advance(s, g, {}, 1. / 120);
    require(s.player.vy < -30 && s.player.dash_available,
            "spring launches and refills dash");
    g.springs.clear();
    g.enemies.push_back({10, 14, 39, 2});
    s = new_state(g);
    s.player.x = 10;
    s.player.y = 39 - kPlayerHeight - .02;
    s.player.vy = 8;
    step = FixedStepper{};
    step.advance(s, g, {}, .02);
    require(s.defeated[0] && s.player.vy < 0 && s.deaths == 0,
            "enemy stomp bounces safely");
    s = new_state(g);
    s.player.x = 10;
    s.player.y = 39;
    step = FixedStepper{};
    step.advance(s, g, {}, 1. / 120);
    require(s.deaths == 1, "enemy side contact resets to checkpoint");
    s = new_state(g);
    s.player.x = 9.3;
    s.player.y = 38.2;
    input = {};
    input.move = 1;
    input.dash_pressed = true;
    step = FixedStepper{};
    step.advance(s, g, input, .02);
    require(s.defeated[0] && s.deaths == 0, "dash defeats patrol");
    SshInput legacy(true);
    legacy.feed("d");
    require(legacy.consume(.01).move == 1, "SSH basic starts movement");
    require(legacy.consume(3).move == 1,
            "SSH basic does not depend on autorepeat");
    legacy.feed("s");
    require(legacy.consume(.01).move == 0, "SSH basic stop is explicit");
    legacy.feed("1\r");
    auto ui = legacy.consume(.01);
    require(ui.select_level == 1 && ui.confirm, "SSH legacy menu controls");
    SshInput keys;
    keys.feed("\x1b[?11u\x1b[100;");
    require(keys.consume(.01).move == 0,
            "partial network sequence is retained");
    keys.feed("1:1u\x1b[32;1:1u");
    auto both = keys.consume(.01);
    require(both.move == 1 && both.jump_held && both.jump_pressed,
            "SSH simultaneous press states");
    keys.feed("\x1b[32;1:3u\x1b[100;1:3u");
    both = keys.consume(.01);
    require(both.move == 0 && !both.jump_held, "SSH releases clear held state");
    keys.feed("\x1b[32;1:1u\x1b[32;1:3u");
    require(keys.consume(.01).jump_pressed,
            "SSH short tap survives same packet release");
    keys.feed("\x1b[100;1:1u\x1b[O");
    require(!keys.focused && keys.consume(.01).move == 0,
            "SSH focus loss clears controls");
    keys.feed("\x1b[I");
    require(keys.focused, "SSH focus regain");
    int maxdeaths = 0;
    for (uint32_t seed = 0; seed < 100; ++seed) {
      auto world = build_procedural(seed);
      auto run = new_state(world);
      FixedStepper fixed;
      for (int tick = 0; tick < 120 * 80 && !run.won; ++tick) {
        auto &p = run.player;
        Controls k;
        k.move = 1;
        k.jump_held = true;
        int foot = int(std::floor(p.y + kPlayerHeight + .05));
        bool trouble = false;
        for (double dx = 1; dx <= 2.5; dx += .5) {
          int x = int(p.x + kPlayerWidth + dx);
          trouble |= world.solid_at(x, foot - 1) || !world.solid_at(x, foot);
        }
        k.jump_pressed = p.grounded && trouble;
        int x = int(p.x + kPlayerWidth + 1);
        k.dash_pressed = !p.grounded && p.vy >= 0 && !world.solid_at(x, 38) &&
                         p.dash_available;
        for (size_t e = 0; e < world.enemies.size(); ++e)
          if (!run.defeated[e]) {
            auto enemy = world.enemies[e].at(run.elapsed);
            if (enemy.x >= p.x - 1 && enemy.x - p.x < 2.5 &&
                std::abs(enemy.y - p.y) < 2.0 && p.dash_available)
              k.dash_pressed = true;
          }
        fixed.advance(run, world, k, 1. / 120);
      }
      if (!run.won) {
        std::cerr << "seed " << seed << " stuck x=" << run.player.x
                  << " y=" << run.player.y << " deaths=" << run.deaths << '\n';
        throw std::runtime_error("generated route completion");
      }
      maxdeaths = std::max(maxdeaths, run.deaths);
    }
    std::cout
        << "PASS 100 seeded routes completed using normal controls; max deaths="
        << maxdeaths << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
