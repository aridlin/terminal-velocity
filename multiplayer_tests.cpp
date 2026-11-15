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
    using namespace ghosts;
    Snapshot a;
    a.room = "friends";
    a.sequence = 2;
    a.level = 1;
    a.seed = 73;
    a.x = 12.345;
    a.y = -2.125;
    a.flags = 3;
    Snapshot decoded;
    auto packet = encode(a);
    check(decode(packet, decoded) && decoded.x == a.x && decoded.y == a.y &&
              decoded.room == a.room && decoded.seed == 73,
          "snapshot roundtrip, signed coordinates");
    packet[0] = 'X';
    check(!decode(packet, decoded), "reject incompatible protocol");
    packet = encode(a);
    put(packet, 44, 0x7fffffffu);
    check(!decode(packet, decoded), "reject out-of-bounds positions");
    packet = encode(a);
    packet[12] = 1;
    check(!decode(packet, decoded), "reject hidden room bytes");
    check(newer(0, UINT32_MAX) && !newer(3, 4) && !newer(4, 4),
          "sequence wrap and reordering");
    Relay relay(0);
    Client first, second, outsider;
    auto port = std::to_string(relay.port());
    first.connect_to("127.0.0.1", port);
    second.connect_to("127.0.0.1", port);
    outsider.connect_to("127.0.0.1", port);
    Snapshot b = a, other = a;
    b.x = 18;
    other.room = "another-room";
    auto pump = [&](double time) {
      first.tick(a, .04, time);
      second.tick(b, .04, time);
      outsider.tick(other, .04, time);
      relay.poll(time);
      first.tick(a, .001, time + .001);
      second.tick(b, .001, time + .001);
      outsider.tick(other, .001, time + .001);
    };
    pump(100);
    pump(100.04);
    check(first.visible().size() == 1 && second.visible().size() == 1 &&
              outsider.visible().empty(),
          "two real UDP clients exchange ghosts; rooms isolated");
    check(std::abs(first.visible()[0].state.x - b.x) < .001 &&
              first.visible()[0].state.id != 0,
          "relay assigns identity and carries positions");
    b.x = 19;
    pump(100.08);
    auto g = first.visible()[0];
    check(g.x > 18 && g.x < 19, "remote movement smoothed between snapshots");
    b.x = 4;
    ++b.epoch;
    pump(100.12);
    check(first.visible()[0].x == 4, "respawn snaps immediately");
    b.flags = 1;
    pump(100.16);
    check(first.visible().empty(), "paused players hide their ghost");
    b.flags = 3;
    b.seed = 74;
    pump(100.20);
    pump(100.24);
    check(second.visible().empty(), "changing seed clears old ghosts");
    first.tick(a, .01, 104);
    check(first.visible().empty(), "silent peers expire after three seconds");
    b.seed = 73;
    pump(104.04);
    pump(104.08);
    check(first.visible().size() == 1 && second.visible().size() == 1,
          "clients rejoin after relay expiry");
    b.level = 2;
    b.seed = 0;
    pump(104.12);
    check(second.visible().empty(), "different courses isolated");

    auto raw = udp_socket();
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(relay.port());
    check(raw != invalid_socket &&
              ::connect(raw, reinterpret_cast<sockaddr *>(&address),
                        sizeof(address)) == 0,
          "raw UDP test endpoint connects");
    Snapshot injected = a;
    injected.id = 9999;
    injected.sequence = 10;
    injected.x = 25;
    packet = encode(injected);
    send_packet(raw, packet.data(), packet.size());
    relay.poll(104.2);
    first.tick(a, .01, 104.201);
    auto visible = first.visible();
    auto found = std::find_if(visible.begin(), visible.end(),
                              [](auto &g) { return g.state.x == 25; });
    check(found != visible.end() && found->state.id != 9999,
          "relay ignores client-supplied identity");
    auto assigned = found->state.id;
    injected.sequence = 9;
    injected.x = 30;
    packet = encode(injected);
    send_packet(raw, packet.data(), packet.size());
    relay.poll(104.24);
    first.tick(a, .01, 104.241);
    visible = first.visible();
    found = std::find_if(visible.begin(), visible.end(), [assigned](auto &g) {
      return g.state.id == assigned;
    });
    check(found != visible.end() && found->state.x == 25,
          "relay rejects stale UDP snapshot");
    std::array<unsigned char, 61> oversized{};
    injected.sequence = 11;
    packet = encode(injected);
    std::copy(packet.begin(), packet.end(), oversized.begin());
    send_packet(raw, oversized.data(), oversized.size());
    relay.poll(104.28);
    first.tick(a, .01, 104.281);
    visible = first.visible();
    found = std::find_if(visible.begin(), visible.end(), [assigned](auto &g) {
      return g.state.id == assigned;
    });
    check(found != visible.end() && found->state.x == 25,
          "relay rejects oversized datagrams");
    close_socket(raw);

    // Reusable refills work during an active dash and reset at checkpoints.
    EngineGrid world(60, 46);
    world.event_level = false;
    world.spawn_x = 10;
    world.spawn_y = 20;
    world.goal = {55, 33, 3, 5};
    world.refills.push_back({11, 20.5});
    auto run = new_state(world);
    Controls c;
    c.move = 1;
    c.dash_pressed = true;
    FixedStepper fixed;
    fixed.advance(run, world, c, 1. / 120);
    check(run.player.dash_available && run.refill_cooldown[0] > 2,
          "airborne refill restores dash without cancelling momentum");
    run.player.dash_available = false;
    fixed.advance(run, world, {}, 1. / 120);
    check(!run.player.dash_available,
          "refill cooldown prevents infinite immediate recharge");
    run.refill_cooldown[0] = .001;
    run.player.x = 10.2;
    fixed.advance(run, world, {}, 1. / 120);
    check(run.player.dash_available, "refill becomes available again");
    run.refill_cooldown[0] = 2;
    reset_player(run);
    check(run.refill_cooldown[0] == 0,
          "checkpoint retry restores refill route");

    EngineGrid wall(60, 46);
    wall.spawn_x = 9.1;
    wall.spawn_y = 20;
    wall.add_solid_rect(10, 10, 2, 25);
    run = new_state(wall);
    fixed = FixedStepper{};
    fixed.advance(run, wall, {}, 1. / 120);
    run.player.x -= .3;
    c = {};
    c.jump_pressed = c.jump_held = true;
    fixed.advance(run, wall, c, 1. / 120);
    check(run.player.vx < -10 && run.player.vy < -20,
          "wall jump accepted just after leaving wall");

    // Complete the authored main route with normal jump and dash inputs.
    auto summit = build_summit();
    run = new_state(summit);
    fixed = FixedStepper{};
    int dashes = 0;
    for (int tick = 0; tick < 120 * 100 && !run.won; ++tick) {
      auto &p = run.player;
      c = {};
      c.move = 1;
      c.jump_held = true;
      int foot = int(std::floor(p.y + kPlayerHeight + .05));
      bool trouble = false;
      for (double dx = 1; dx <= 2.5; dx += .5) {
        int x = int(p.x + kPlayerWidth + dx);
        trouble |= summit.solid_at(x, foot - 1) ||
                   summit.hazard_at(x, foot - 1) || !summit.solid_at(x, foot);
      }
      c.jump_pressed = p.grounded && trouble;
      c.dash_pressed = !p.grounded && p.vy >= 0 &&
                       !summit.solid_at(int(p.x + kPlayerWidth + 1), 38) &&
                       p.dash_available;
      if (c.dash_pressed)
        ++dashes;
      fixed.advance(run, summit, c, 1. / 120);
    }
    if (!run.won)
      std::cerr << "summit stuck x=" << run.player.x << " y=" << run.player.y
                << " deaths=" << run.deaths << '\n';
    check(run.won && dashes >= 6,
          "six-room precision course completable with normal controls");
    std::cout << "summit deaths=" << run.deaths << " dashes=" << dashes << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
