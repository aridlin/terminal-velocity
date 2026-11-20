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

    EngineGrid open(80,60);
    open.spawn_x=20;open.spawn_y=24;
    auto quick_dash=new_state(open);quick_dash.player.facing=-1;
    Controls chord;chord.move_pressed=1;chord.aim_pressed=-1;chord.dash_pressed=true;
    FixedStepper short_frame;
    short_frame.advance(quick_dash,open,chord,.001);
    require(quick_dash.player.dash_available,"short frame waits for a physics tick");
    short_frame.advance(quick_dash,open,{},.01);
    require(!quick_dash.player.dash_available&&quick_dash.player.vx>0&&quick_dash.player.vy<0,
      "released direction steers a queued dash at the next physics tick");

    auto impulse=new_state(open),expected=impulse;
    FixedStepper impulse_step,expected_step;
    Controls tap_right;tap_right.move_pressed=1;
    Controls hold_right;hold_right.move=1;
    impulse_step.advance(impulse,open,tap_right,3./120.);
    expected_step.advance(expected,open,hold_right,1./120.);
    expected_step.advance(expected,open,{},2./120.);
    require(impulse.player.x>20&&std::abs(impulse.player.x-expected.player.x)<1e-9,
      "a released direction moves for just its first physics tick");
    SshInput legacy(true);
    legacy.feed("d");
    require(legacy.consume(.01).move == 1, "SSH basic starts movement");
    require(legacy.consume(3).move == 1,
            "SSH basic does not depend on autorepeat");
    legacy.feed("s");
    require(legacy.consume(.01).move == 0, "SSH basic stop is explicit");
    legacy.feed("i");
    auto vertical=legacy.consume(.01);
    require(vertical.dash_pressed&&vertical.aim_y==-1&&vertical.move==0,
            "SSH basic has an explicit upward dash for wall bounces");
    require(legacy.consume(.01).aim_y==0&&!legacy.consume(.01).dash_pressed,
            "SSH basic vertical dash is an edge, not a latched repeat");
    legacy.feed("do");vertical=legacy.consume(.01);
    require(vertical.dash_pressed&&vertical.aim_y==1&&vertical.move==1,
            "SSH basic combines latched direction with a downward diagonal dash");
    legacy.feed("s");legacy.consume(.01);
    legacy.feed("1\r");
    auto ui = legacy.consume(.01);
    require(ui.select_level == 1 && ui.confirm, "SSH legacy menu controls");
    legacy.feed("\x1b[6~");
    require(legacy.consume(.01).tour_page==1,"SSH basic Page Down reaches Tour navigation");
    legacy.feed("\x1b[M");
    require(!legacy.consume(.01).confirm, "CSI M is not keypad Enter");
    legacy.feed("\x1bO");
    require(!legacy.consume(.01).confirm, "partial keypad Enter waits for its final byte");
    legacy.feed("M");
    require(legacy.consume(.01).confirm, "SSH basic application keypad Enter confirms");
    SshInput keys;
    keys.feed("\x1b[?11u\x1b[57414;1:1u\x1b[57414;1:3u");
    require(keys.consume(.01).confirm, "SSH enhanced keypad Enter keeps a short tap");
    require(!keys.consume(.01).confirm, "SSH enhanced keypad Enter is a single edge");
    keys.feed("\x1b[57414;1:2u\x1b[57414;1:3u");
    require(!keys.consume(.01).confirm, "SSH enhanced keypad Enter ignores repeat and release");
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
    keys.feed("\x1b[115;1:1u\x1b[115;1:3u");
    auto short_down = keys.consume(.01);
    require(short_down.fast_fall_pressed && !short_down.fast_fall,
            "enhanced SSH short Down tap survives the release packet");
    keys.feed("\x1b[100;1:1u\x1b[115;1:1u\x1b[120;1:1u"
              "\x1b[120;1:3u\x1b[115;1:3u\x1b[100;1:3u");
    auto tap_dash=keys.consume(.001);
    require(tap_dash.move==0&&!tap_dash.fast_fall&&tap_dash.aim_y==0&&
            tap_dash.move_pressed==1&&tap_dash.aim_pressed==1&&tap_dash.dash_pressed,
            "enhanced SSH preserves both released dash directions");
    auto remote_dash=new_state(open);
    remote_dash.player.facing=-1;
    FixedStepper remote_frame;
    remote_frame.advance(remote_dash,open,tap_dash,.001);
    auto idle=keys.consume(.009);
    require(!idle.move_pressed&&!idle.aim_pressed&&!idle.dash_pressed,
            "enhanced SSH directional edges only survive one input sample");
    remote_frame.advance(remote_dash,open,idle,.009);
    require(remote_dash.player.vx>20&&remote_dash.player.vy>20,
            "the full server input path launches a quick downward diagonal dash");
    keys.feed("\x1b[97;1:1u\x1b[100;1:1u\x1b[119;1:1u\x1b[115;1:1u");
    auto opposed=keys.consume(.01);
    require(opposed.move==0&&opposed.aim_y==0&&opposed.move_pressed==0&&opposed.aim_pressed==0,
            "opposing held directions cancel queued steering and aim");
    keys.feed("\x1b[97;1:3u\x1b[100;1:3u\x1b[119;1:3u\x1b[115;1:3u");
    keys.consume(.01);
    keys.feed("\x1b[57354;1:1u\x1b[57354;1:3u");
    require(keys.consume(.01).tour_page==-1,"SSH enhanced Page Up preserves the press before release");
    keys.feed("\x1b[100;1:1u\x1b[O");
    require(!keys.focused && keys.consume(.01).move == 0,
            "SSH focus loss clears controls");
    keys.feed("\x1b[I");
    require(keys.focused, "SSH focus regain");
    for(bool basic:{true,false}) {
      SshInput bounce(basic);
      bounce.feed(basic?"x \x1b[Oa\x1b[":"\x1b[120;1:1u\x1b[32;1:1u\x1b[O\x1b[97;1:1u\x1b[");
      bounce.feed(basic?"Id":"I\x1b[100;1:1u");
      const auto recovered=bounce.consume(.01);
      require(bounce.focused&&recovered.focus_lost&&!recovered.dash_pressed&&
              !recovered.jump_pressed&&recovered.move==1&&!bounce.consume(.01).focus_lost,
              "a split SSH focus bounce preserves the loss once and keeps fresh movement");
    }
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
