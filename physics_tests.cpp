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
    auto adventure = flat();
    adventure.adventure_level = true;
    auto land_and_dash = [](const EngineGrid &floor) {
      auto state = new_state(floor);
      state.player.y = 40 - kPlayerHeight - .03;
      state.player.vy = 10;
      state.player.dash_available = false;
      world_step(state, floor, {}, 1./120);
      return state;
    };
    auto landed = land_and_dash(adventure);
    check(landed.player.grounded && !landed.player.can_dash(),
          "landing begins a short dash cooldown");
    Controls dash;
    dash.move = 1;
    dash.dash_pressed = true;
    world_step(landed, adventure, dash, 1./120);
    check(landed.player.dash_time_left == 0 && landed.player.dash_buffer_left > 0,
          "a dash pressed on landing is buffered");
    for (int i = 0; i < 11; ++i)
      world_step(landed, adventure, {}, 1./120);
    check(landed.player.dash_time_left > 0 && landed.player.vx > 20,
          "the buffered dash fires when the landing cooldown expires");
    auto vertical = land_and_dash(adventure);
    Controls up;
    up.aim_y = -1;
    up.dash_pressed = true;
    world_step(vertical, adventure, up, 1./120);
    for (int i = 0; i < 11; ++i)
      world_step(vertical, adventure, {}, 1./120);
    check(vertical.player.dash_time_left > 0 && std::abs(vertical.player.vx) < 1e-6 &&
          vertical.player.vy < -20,
          "a briefly tapped upward dash keeps its aim after landing cooldown");
    auto reaimed=land_and_dash(adventure);
    world_step(reaimed, adventure, up, 1./120);
    Controls diagonal;
    diagonal.move=-1;
    diagonal.aim_y=-1;
    diagonal.dash_pressed=true;
    world_step(reaimed, adventure, diagonal, 1./120);
    for (int i=0;i<10;++i)
      world_step(reaimed, adventure, {}, 1./120);
    check(reaimed.player.dash_time_left>0&&reaimed.player.vx<-20&&reaimed.player.vy<-20,
          "a second dash tap replaces the buffered direction with its diagonal aim");
    check(land_and_dash(g).player.can_dash(), "classic courses retain their landing timing");
    Controls combo;
    combo.move = 1;
    combo.dash_pressed = combo.jump_pressed = true;
    auto step_combo = [&](const EngineGrid &course, Controls input, bool reference = false) {
      auto state = new_state(course);
      state.reference_movement = reference;
      ticks(state, course, {}, 2);
      world_step(state, course, input, 1./120);
      return state.player;
    };
    auto buffered_combo=land_and_dash(adventure);
    world_step(buffered_combo,adventure,combo,1./120);
    check(buffered_combo.player.grounded&&buffered_combo.player.landing_dash_left>0&&
          buffered_combo.player.dash_time_left==0&&buffered_combo.player.dash_jump_left==0,
          "a paired Dash and Jump waits for the landing cooldown");
    for(int i=0;i<11;++i)world_step(buffered_combo,adventure,{},1./120);
    check(buffered_combo.player.dash_jump_left>0&&buffered_combo.player.vx>20&&buffered_combo.player.vy<0,
          "the paired landing inputs launch a dash jump after the cooldown");
    auto buffered_jump=land_and_dash(adventure);
    world_step(buffered_jump,adventure,{.jump_pressed=true},1./120);
    check(!buffered_jump.player.grounded&&buffered_jump.player.vy<0,
          "a jump without Dash still takes off during the landing cooldown");
    const auto launched = step_combo(adventure, combo);
    check(launched.dash_jump_left > 0 && launched.vx > 20 && launched.vy < 0,
          "same-tick Dash and Jump launch a grounded dash jump");
    combo.aim_y = -1;
    const auto diagonal_launch = step_combo(adventure, combo);
    check(diagonal_launch.dash_jump_left == 0 && diagonal_launch.dash_time_left > 0 && diagonal_launch.vy < 0,
          "aiming upward keeps a diagonal dash when Jump is pressed");
    combo.aim_y = 0;
    check(step_combo(g, combo).dash_jump_left == 0 && step_combo(adventure, combo, true).dash_jump_left == 0,
          "classic and reference movement retain same-tick launch behavior");
    auto lift = adventure;
    lift.platforms.push_back({{4,36,8,.5},0,.01,3,0});
    auto on_lift = new_state(lift);
    on_lift.player.y = 36 - kPlayerHeight - .02;
    on_lift.player.vy = 12;
    on_lift.player.dash_available = false;
    world_step(on_lift, lift, {}, 1./120);
    check(on_lift.player.support == 0 && !on_lift.player.can_dash(),
          "landing on a moving platform also starts the cooldown");
    for (int i = 0; i < 15; ++i) world_step(on_lift, lift, {}, 1./120);
    check(on_lift.player.support == 0 && on_lift.player.can_dash(), "the lift rider can dash after the cooldown");
    auto dropping = on_lift;
    FixedStepper drop_step;
    Controls quick_down;
    quick_down.fast_fall_pressed = true;
    drop_step.advance(dropping, lift, quick_down, .001);
    check(dropping.player.support == 0, "a short Down tap waits for the next physics step");
    drop_step.advance(dropping, lift, {}, 1./120);
    check(dropping.player.support == -1 && !dropping.player.grounded && dropping.player.drop_left > 0,
          "a short Down tap drops through a moving platform even after release");
    world_step(on_lift, lift, combo, 1./120);
    check(on_lift.player.dash_jump_left > 0 && on_lift.player.vy < 0,
          "same-tick dash jumps work off moving platforms");
    adventure.add_solid_rect(10, 24, 4, 8);
    for(int side:{1,-1}) {
      auto approach = new_state(adventure);
      approach.player.x = side>0?10-kPlayerWidth-.03:14+.03;
      approach.player.y = 23.45;
      approach.player.vy = 4;
      Controls toward;
      toward.move = side;
      world_step(approach, adventure, toward, 1./120);
      check(approach.player.ledge && approach.player.ledge->side == side,
            "falling toward a clear ledge catches it from either side");
      const auto hanging = approach;
      auto tapped = hanging;
      Controls down_tap = toward;
      down_tap.fast_fall_pressed = true;
      FixedStepper tap_step;
      tap_step.advance(tapped, adventure, down_tap, .001);
      tap_step.advance(tapped, adventure, toward, 1./120);
      check(!tapped.player.ledge && tapped.player.y > hanging.player.y,
            "a Down tap drops from either ledge even while holding toward it");
      world_step(approach, adventure, {}, 1./120);
      check(!approach.player.ledge && approach.player.y > hanging.player.y,
            "releasing the direction drops from a ledge");
      approach = hanging;
      Controls jump_on_release;
      jump_on_release.jump_pressed=jump_on_release.jump_held=true;
      world_step(approach, adventure, jump_on_release, 1./120);
      check(approach.player.ledge&&approach.player.ledge_climb_left>0.,
            "a jump pressed as the direction is released starts the ledge climb");
      for(int i=0;i<28;++i)world_step(approach,adventure,{},1./120);
      check(!approach.player.ledge&&approach.player.grounded&&approach.deaths==0,
            "the released-direction climb still finishes safely");
      approach=hanging;
      jump_on_release.fast_fall=true;
      world_step(approach,adventure,jump_on_release,1./120);
      check(!approach.player.ledge,"Down still drops from a ledge even when Jump is also pressed");
      approach=hanging;
      jump_on_release.fast_fall=false;
      jump_on_release.move=-side;
      world_step(approach,adventure,jump_on_release,1./120);
      check(!approach.player.ledge,"steering away still drops from a ledge even when Jump is also pressed");
      approach = hanging;
      toward.jump_pressed = toward.jump_held = true;
      world_step(approach, adventure, toward, 1./120);
      check(approach.player.ledge && approach.player.ledge_climb_left > 0 &&
            approach.player.y < hanging.player.y && approach.player.x == hanging.player.x,
            "jump starts a climb by lifting the courier before crossing");
      for(int i=0;i<21;++i) world_step(approach,adventure,{},1./120);
      check(approach.player.ledge && (approach.player.x - hanging.player.x)*side > .05,
            "the climb travels sideways across the ledge over several ticks");
      auto queued_jump=approach;
      world_step(queued_jump,adventure,{.jump_held=true,.jump_pressed=true},1./120);
      for(int i=0;i<8;++i)world_step(queued_jump,adventure,{.jump_held=true},1./120);
      check(!queued_jump.player.ledge&&!queued_jump.player.grounded&&
            queued_jump.player.vy<0&&queued_jump.player.rect().bottom()<24.&&queued_jump.deaths==0,
            "a new jump buffered near the top of either ledge takes off after the climb");
      for(int i=0;i<7;++i) world_step(approach,adventure,{},1./120);
      check(!approach.player.ledge && approach.player.grounded && approach.deaths == 0 &&
            std::abs(approach.player.rect().bottom() - 24.) < 1e-6,
            "the courier stands safely on top after climbing");
    }
    auto blocked_ledge = adventure;
    blocked_ledge.add_solid_rect(10,22,4,1);
    auto blocked = new_state(blocked_ledge);
    blocked.player.x=10-kPlayerWidth-.03;blocked.player.y=23.45;blocked.player.vy=4;
    Controls towards_block;
    towards_block.move=1;
    world_step(blocked,blocked_ledge,towards_block,1./120);
    check(!blocked.player.ledge,"a ledge without headroom keeps normal wall physics");
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
    const double fall_time=s.elapsed;
    Controls falling;
    falling.move=1;
    step.advance(s, course, falling, .1);
    check(s.deaths==1&&s.player.x==s.spawn_x&&s.player.y==s.spawn_y&&s.player.dash_available&&
              step.accumulator==0.&&std::abs(s.elapsed-fall_time-1./120)<1e-8,
          "a hazard respawn ends a slow frame at the checkpoint");
    step.advance(s, course, falling, 1./120);
    check(s.deaths==1&&s.player.x>s.spawn_x,"held movement resumes on the next frame after a fall");
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
