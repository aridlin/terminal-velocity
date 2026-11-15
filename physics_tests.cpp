#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <fstream>
#include <stdexcept>
void check(bool ok, const char *what) {
  if (!ok)
    throw std::runtime_error(what);
  std::cout << "PASS " << what << '\n';
}
EngineGrid flat() {
  EngineGrid g(200, 60);
  g.add_solid_rect(0, 40, 200, 20);
  g.spawn_x = 5;
  g.spawn_y = 40 - kPlayerHeight;
  g.goal = {190, 30, 3, 10};
  return g;
}
void ticks(GameState &s, const EngineGrid &g, Controls c, int n) {
  for (int i = 0; i < n; ++i) {
    update_step(s, g, c, 1. / 120);
    c.jump_pressed = c.dash_pressed = c.restart = false;
  }
}
double jump_height(bool held) {
  auto g = flat();
  auto s = new_state(g);
  ticks(s, g, {}, 2);
  double base = s.player.y, miny = base;
  Controls c;
  c.jump_pressed = true;
  c.jump_held = held;
  for (int i = 0; i < 150; ++i) {
    ticks(s, g, c, 1);
    c.jump_pressed = false;
    miny = std::min(miny, s.player.y);
  }
  return base - miny;
}
GameState replay(int hz) {
  auto g = flat();
  auto s = new_state(g);
  FixedStepper step;
  Controls c;
  c.move = 1;
  for (int i = 0; i < hz * 3; ++i) {
    c.jump_pressed = i == hz;
    c.jump_held = i >= hz && i < hz * 2;
    step.advance(s, g, c, 1. / hz);
  }
  return s;
}
int main() {
  try {
    auto g = flat();
    auto s = new_state(g);
    ticks(s, g, {}, 120);
    check(s.player.grounded &&
              std::abs(s.player.y - (40 - kPlayerHeight)) < 1e-6,
          "stable resting contact");
    Controls c;
    c.move = 1;
    ticks(s, g, c, 120);
    check(std::abs(s.player.vx - kMaxRunSpeed) < 1e-8,
          "run speed reaches target");
    ticks(s, g, {}, 30);
    check(std::abs(s.player.vx) < 1e-8, "release stops ground motion");
    double full = jump_height(true), shortjump = jump_height(false);
    check(full > 4 && full < 5 && shortjump < full * .75,
          "variable jump height");
    std::cout << "jump full=" << full << " short=" << shortjump << '\n';
    auto a = replay(30), b = replay(60), d = replay(144);
    check(std::abs(a.player.x - b.player.x) < 1e-7 &&
              std::abs(a.player.x - d.player.x) < 1e-7 &&
              std::abs(a.player.y - d.player.y) < 1e-7,
          "identical physics at 30 / 60 / 144 Hz");
    s = new_state(g);
    ticks(s, g, {}, 1);
    FixedStepper step;
    c = {};
    c.jump_pressed = true;
    step.advance(s, g, c, .001);
    c.jump_pressed = false;
    step.advance(s, g, c, .009);
    check(s.player.vy < 0, "short tap retained between ticks");
    s = new_state(g);
    s.player.y = 20;
    s.player.vx = 8;
    c = {};
    c.dash_pressed = true;
    c.move = 1;
    c.aim_y = -1;
    ticks(s, g, c, 1);
    check(std::abs(std::hypot(s.player.vx, s.player.vy) - kDashSpeed) < 1e-6,
          "diagonal dash normalized");
    c.dash_pressed = false;
    ticks(s, g, c, 26);
    check(!s.player.dash_available, "air dash cannot recharge in midair");
    auto wall = flat();
    wall.add_solid_rect(10, 20, 1, 20);
    s = new_state(wall);
    s.player.x = 7;
    s.player.y = 30;
    c = {};
    c.move = 1;
    c.dash_pressed = true;
    ticks(s, wall, c, 24);
    check(s.player.x <= 10 - kPlayerWidth + 1e-6,
          "dash cannot tunnel through one-tile wall");
    s = new_state(wall);
    s.player.x = 10 - kPlayerWidth;
    s.player.y = 32;
    s.player.vy = 2;
    c = {};
    c.move = 1;
    c.jump_pressed = true;
    c.jump_held = true;
    ticks(s, wall, c, 10);
    check(s.player.vx < 0 && s.player.x < 8.5,
          "wall jump survives held direction into wall");
    auto edge =
        flat(); // Explicit recently-left-ground state isolates coyote behavior.
    s = new_state(edge);
    s.player.y = 30;
    s.player.coyote_left = .06;
    c = {};
    c.jump_pressed = c.jump_held = true;
    ticks(s, edge, c, 1);
    check(s.player.vy < -20, "coyote jump accepted after leaving ledge");
    s = new_state(g);
    s.player.y = 40 - kPlayerHeight - .2;
    s.player.vy = 10;
    c = {};
    c.jump_pressed = c.jump_held = true;
    ticks(s, g, c, 8);
    check(s.player.vy < 0, "jump buffered just before landing");
    auto course = build_level();
    s = new_state(course);
    s.player.x = 61;
    s.player.y = 38 - kPlayerHeight;
    step = FixedStepper{};
    step.advance(s, course, {}, 1. / 120);
    check(s.checkpoint == 1, "safe checkpoint activates");
    s.player.y = 60;
    step.advance(s, course, {}, 1. / 120);
    check(s.deaths == 1 && s.player.x == 61, "death respawns at checkpoint");
    s = new_state(course);
    step = FixedStepper{};
    int jumps = 0, dashes = 0;
    std::ofstream trace;
    trace << "tick,x,y,jump,dash\n";
    // A modest look-ahead player uses ordinary inputs; no teleporting or
    // disabled hazards.
    for (int i = 0; i < 120 * 90 && !s.won; ++i) {
      c = {};
      c.move = 1;
      c.jump_held = true;
      auto &p = s.player;
      int foot = int(std::floor(p.y + kPlayerHeight + .05));
      bool trouble = false;
      for (double dx = 1; dx <= 2.5; dx += .5) {
        int x = int(p.x + kPlayerWidth + dx);
        trouble |= course.solid_at(x, foot - 1) ||
                   course.hazard_at(x, foot - 1) || !course.solid_at(x, foot);
      }
      if (p.grounded && trouble) {
        c.jump_pressed = true;
        ++jumps;
      }
      int x = int(p.x + kPlayerWidth + 1);
      if (!p.grounded && p.vy >= 0 && !course.solid_at(x, 38) &&
          p.dash_available) {
        c.dash_pressed = true;
        ++dashes;
      }
      step.advance(s, course, c, 1. / 120);
      trace << i << ',' << p.x << ',' << p.y << ',' << c.jump_pressed << ','
            << c.dash_pressed << '\n';
    }
    std::cout << "Route x=" << s.player.x << " deaths=" << s.deaths
              << " jumps=" << jumps << " dashes=" << dashes
              << " cp=" << s.checkpoint << '\n';
    check(s.won && s.deaths == 0 && s.checkpoint == 2,
          "entire course beatable with ordinary inputs and zero deaths");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
