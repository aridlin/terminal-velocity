#include "ghost_net.hpp"
#include <csignal>
#ifndef _WIN32
#include <fcntl.h>
#include <fstream>
#include <linux/input.h>
#include <map>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr double kTargetFps = 50.0;
constexpr double kDefaultCellAspect = 2.0;
constexpr double kInputHold = 0.18;
constexpr double kWorldUnitsPerRenderCol = 0.30;

constexpr double kPlayerWidth = 0.90;
constexpr double kPlayerHeight = 1.80;
constexpr double kMoveAccel = 126.0;
constexpr double kAirAccel = 82.0;
constexpr double kApexAccelBonus = 30.0;
constexpr double kMaxRunSpeed = 11.8;
constexpr double kGroundFriction = 88.0;
constexpr double kGravity = 58.0;
constexpr double kApexGravityScale = 0.42;
constexpr double kJumpSpeed = 21.8;
constexpr double kJumpCutGravity = 58.0;
constexpr double kCoyoteTime = 0.125;
constexpr double kJumpBuffer = 0.15;
constexpr double kDashBuffer = 0.13;
constexpr double kMaxFallSpeed = 38.0;
constexpr double kFastFallSpeed = 47.0;
constexpr double kWallSlideSpeed = 6.8;
constexpr double kWallJumpX = 16.5;
constexpr double kWallJumpY = 22.5;
constexpr double kDashSpeed = 32.0;
constexpr double kDashTime = 0.18;

constexpr int kWorldWidth = 184;
constexpr int kWorldHeight = 46;

double clamp(double value, double lo, double hi) {
  return std::max(lo, std::min(hi, value));
}

double approach_zero(double value, double amount) {
  if (std::abs(value) <= amount) {
    return 0.0;
  }
  return value - std::copysign(amount, value);
}

struct Rect {
  double x = 0.0;
  double y = 0.0;
  double w = 0.0;
  double h = 0.0;

  double left() const { return x; }
  double right() const { return x + w; }
  double top() const { return y; }
  double bottom() const { return y + h; }

  bool intersects(const Rect &other) const {
    return left() < other.right() && right() > other.left() &&
           top() < other.bottom() && bottom() > other.top();
  }
};

struct Player {
  double x = 0.0;
  double y = 0.0;
  double vx = 0.0;
  double vy = 0.0;
  bool grounded = false;
  bool dash_available = true;
  int facing = 1;
  double coyote_left = 0.0;
  double jump_buffer_left = 0.0;
  double dash_buffer_left = 0.0;
  double dash_time_left = 0.0;
  double wall_lock_left = 0.0;
  double wall_coyote_left = 0;
  int last_wall = 0;
  int support = -1;
  double drop_left = 0.0;

  Rect rect() const { return Rect{x, y, kPlayerWidth, kPlayerHeight}; }
};

struct Controls {
  int move = 0;
  bool jump_held = false;
  bool jump_pressed = false;
  bool dash_pressed = false;
  bool fast_fall = false;
  bool restart = false;
  bool quit = false;
  int aim_y = 0;
  bool pause = false, journal = false, confirm = false, debug = false,
       new_run = false;
  int select_level = 0;
  bool next_seed = false, menu_request = false;
};

struct HeldKeys {
  bool left = false;
  bool right = false;
  bool up = false;
  bool down = false;
  bool jump = false;
  bool jump_pressed = false;
  bool dash = false;
  bool dash_pressed = false;
  bool restart_pressed = false;
  bool quit_pressed = false;

  Controls controls() const {
    return Controls{
        static_cast<int>(right) - static_cast<int>(left),
        jump,
        jump_pressed,
        dash_pressed,
        down,
        restart_pressed,
        quit_pressed,
        static_cast<int>(down) - static_cast<int>(up),
    };
  }
};

struct InputState {
  double left = 0.0;
  double right = 0.0;
  double jump = 0.0;
  double down = 0.0;
  double up = 0.0;
  double dash = 0.0;
  bool jump_pressed = false;
  bool dash_pressed = false;
  bool restart_pressed = false;
  bool quit_pressed = false;

  void begin_frame() {
    jump_pressed = false;
    dash_pressed = false;
    restart_pressed = false;
    quit_pressed = false;
  }

  void press(char key) {
    switch (key) {
    case 'a':
      left = kInputHold;
      break;
    case 'd':
      right = kInputHold;
      break;
    case 's':
      down = kInputHold;
      break;
    case 'i':
      up = kInputHold;
      break;
    case 'w':
    case ' ':
      if (jump <= 0.0) {
        jump_pressed = true;
      }
      jump = kInputHold;
      up = kInputHold;
      break;
    case 'x':
    case 'k':
      if (dash <= 0.0) {
        dash_pressed = true;
      }
      dash = kInputHold;
      break;
    case 'r':
      restart_pressed = true;
      break;
    case 'q':
      quit_pressed = true;
      break;
    default:
      break;
    }
  }

  Controls controls() const {
    return Controls{
        static_cast<int>(right > 0.0) - static_cast<int>(left > 0.0),
        jump > 0.0,
        jump_pressed,
        dash_pressed,
        down > 0.0,
        restart_pressed,
        quit_pressed,
        static_cast<int>(down > 0.0) - static_cast<int>(up > 0.0),
    };
  }

  void decay(double dt) {
    left = std::max(0.0, left - dt);
    right = std::max(0.0, right - dt);
    jump = std::max(0.0, jump - dt);
    down = std::max(0.0, down - dt);
    up = std::max(0.0, up - dt);
    dash = std::max(0.0, dash - dt);
  }
};

inline const std::array<std::pair<double, double>, 15> kSweets = {{{15, 33},
                                                                   {23, 32},
                                                                   {35, 32},
                                                                   {40, 28},
                                                                   {56, 33},
                                                                   {70, 33},
                                                                   {80, 32},
                                                                   {90, 32},
                                                                   {98, 28},
                                                                   {112, 33},
                                                                   {127, 32},
                                                                   {135, 31},
                                                                   {147, 32},
                                                                   {153, 28},
                                                                   {169, 33}}};

struct GameState {
  Player player;
  double spawn_x = 0.0;
  double spawn_y = 0.0;
  double camera_x = 0.0;
  double camera_y = 0.0;
  int deaths = 0;
  int checkpoint = 0;
  std::vector<bool> collected;
  std::vector<bool> defeated;
  std::vector<double> refill_cooldown;
  std::vector<ghosts::Ghost> ghosts;
  bool multiplayer = false;
  bool ssh = false, ssh_basic = false;
  double elapsed = 0, notice = 0;
  bool menu = false, paused = false, journal = false, debug = false;
  int sweets_count() const {
    return std::count(collected.begin(), collected.end(), true);
  }
  bool won = false;
  double best_speed = 0.0;
  std::string input_source = "terminal";
};

enum class InputMode {
  Evdev,
  Terminal,
  Auto,
  Ssh,
  SshBasic,
};

struct MovingPlatform {
  Rect base;
  double dx = 0, dy = 0, period = 4, phase = 0;
  Rect at(double t) const {
    double a = std::sin((t / period + phase) * 6.283185307179586);
    return {base.x + dx * a, base.y + dy * a, base.w, base.h};
  }
};
struct Patrol {
  double left, right, y, speed = 2;
  Rect at(double t) const {
    double w = right - left;
    double z = std::fmod(t * speed, 2 * w);
    return {left + (z < w ? z : 2 * w - z), y, 1.1, 1.0};
  }
};
class EngineGrid {
public:
  EngineGrid(int width, int height)
      : width_(width), height_(height), solid_(width * height, false),
        hazard_(width * height, false) {}

  int width() const { return width_; }
  int height() const { return height_; }

  void add_solid_rect(int x, int y, int w, int h) {
    for (int row = y; row < y + h; ++row) {
      for (int col = x; col < x + w; ++col) {
        if (inside(col, row)) {
          solid_[index(col, row)] = true;
        }
      }
    }
  }

  void add_hazard_rect(int x, int y, int w, int h = 1) {
    for (int row = y; row < y + h; ++row) {
      for (int col = x; col < x + w; ++col) {
        if (inside(col, row)) {
          hazard_[index(col, row)] = true;
        }
      }
    }
  }

  bool solid_at(int col, int row) const {
    if (col < 0 || col >= width_) {
      return true;
    }
    if (row < 0 || row >= height_) {
      return false;
    }
    return solid_[index(col, row)];
  }

  bool hazard_at(int col, int row) const {
    if (!inside(col, row)) {
      return false;
    }
    return hazard_[index(col, row)];
  }

  std::vector<Rect> solid_rects_near(const Rect &rect) const {
    std::vector<Rect> rects;
    const int min_col = static_cast<int>(std::floor(rect.left())) - 1;
    const int max_col = static_cast<int>(std::floor(rect.right())) + 1;
    const int min_row = static_cast<int>(std::floor(rect.top())) - 1;
    const int max_row = static_cast<int>(std::floor(rect.bottom())) + 1;
    for (int row = min_row; row <= max_row; ++row) {
      for (int col = min_col; col <= max_col; ++col) {
        if (solid_at(col, row)) {
          rects.push_back(Rect{static_cast<double>(col),
                               static_cast<double>(row), 1.0, 1.0});
        }
      }
    }
    return rects;
  }

  std::vector<Rect> hazard_rects_near(const Rect &rect) const {
    std::vector<Rect> rects;
    const int min_col = static_cast<int>(std::floor(rect.left())) - 1;
    const int max_col = static_cast<int>(std::floor(rect.right())) + 1;
    const int min_row = static_cast<int>(std::floor(rect.top())) - 1;
    const int max_row = static_cast<int>(std::floor(rect.bottom())) + 1;
    for (int row = min_row; row <= max_row; ++row) {
      for (int col = min_col; col <= max_col; ++col) {
        if (hazard_at(col, row)) {
          rects.push_back(Rect{static_cast<double>(col),
                               static_cast<double>(row) + 0.35, 1.0, 0.65});
        }
      }
    }
    return rects;
  }

  bool event_level = true;
  bool summit = false;
  uint32_t seed = 0;
  std::vector<std::pair<double, double>> sweets;
  std::vector<double> checkpoints;
  std::vector<MovingPlatform> platforms;
  std::vector<Patrol> enemies;
  std::vector<Rect> springs;
  std::vector<std::pair<double, double>> refills;
  double spawn_x = 5.0;
  double spawn_y = 36.0 - kPlayerHeight;
  Rect goal{173.0, 13.0, 3.0, 5.0};

private:
  bool inside(int col, int row) const {
    return col >= 0 && col < width_ && row >= 0 && row < height_;
  }

  int index(int col, int row) const { return row * width_ + col; }

  int width_ = 0;
  int height_ = 0;
  std::vector<bool> solid_;
  std::vector<bool> hazard_;
};

EngineGrid build_level() {
  EngineGrid grid(kWorldWidth, kWorldHeight);
  grid.spawn_x = 4.0;
  grid.spawn_y = 38.0 - kPlayerHeight;
  grid.goal = Rect{177.0, 33.0, 3.0, 5.0};
  grid.sweets.assign(kSweets.begin(), kSweets.end());
  grid.checkpoints = {4, 61, 120};
  // Three short chapters. The main route is forgiving; upper pastries are
  // optional.
  for (auto r : std::vector<std::array<int, 2>>{{0, 22},
                                                {26, 20},
                                                {51, 26},
                                                {83, 16},
                                                {105, 27},
                                                {139, 18},
                                                {163, 21}})
    grid.add_solid_rect(r[0], 38, r[1], 8);
  grid.add_solid_rect(13, 36, 5, 2);
  grid.add_solid_rect(32, 35, 6, 3);
  grid.add_solid_rect(55, 36, 4, 2);
  grid.add_solid_rect(68, 36, 5, 2);
  grid.add_solid_rect(88, 35, 5, 3);
  grid.add_solid_rect(110, 36, 4, 2);
  grid.add_solid_rect(125, 35, 5, 3);
  grid.add_solid_rect(145, 35, 5, 3);
  grid.add_solid_rect(168, 36, 4, 2);
  // Optional rooftop routes and dash refills in the GUI.
  grid.add_solid_rect(38, 31, 5, 1);
  grid.add_solid_rect(96, 31, 5, 1);
  grid.add_solid_rect(151, 31, 5, 1);
  grid.add_hazard_rect(94, 37, 3);
  grid.add_hazard_rect(152, 37, 3);
  return grid;
}

// Seeded rooms use bounded jumps and safe checkpoint bays. Optional upper lanes
// carry the moving platforms and risk/reward pickups; the base route stays
// legible.
EngineGrid build_procedural(uint32_t seed) {
  constexpr int rooms = 8, span = 28;
  EngineGrid g(rooms * span + 24, 46);
  g.event_level = false;
  g.seed = seed;
  g.spawn_x = 4;
  g.spawn_y = 38 - kPlayerHeight;
  g.goal = {double(g.width() - 8), 33, 3, 5};
  std::mt19937 random(seed);
  g.add_solid_rect(0, 38, 8, 8);
  g.checkpoints = {4};
  for (int room = 0; room < rooms; ++room) {
    int x = 8 + room * span, variant = int(random() % 4),
        height = 1 + int(random() % 2), gap = 4 + int(random() % 2);
    g.add_solid_rect(x, 38, span - gap, 8);
    g.add_solid_rect(x + 7, 38 - height, 4, height);
    g.sweets.push_back({double(x + 8), double(35 - height)});
    g.sweets.push_back({double(x + span - gap - 1), 33});
    if (room > 0 && room % 2 == 0)
      g.checkpoints.push_back(x + 2);
    if (variant == 0) {
      g.add_solid_rect(x + 12, 33, 4, 1);
      g.add_solid_rect(x + 18, 30, 4, 1);
      g.sweets.push_back({double(x + 19), 27.8});
    }
    if (variant == 1) {
      g.platforms.push_back(
          {{double(x + 14), 33.5, 4, .5}, 3, 0, 3.8, double(room) / 8});
      g.sweets.push_back({double(x + 16), 30.5});
      g.refills.push_back({double(x + 20), 33});
    }
    if (variant == 2) {
      g.platforms.push_back(
          {{double(x + 14), 33, 4, .5}, 0, 2, 4.8, double(room) / 8});
      g.add_solid_rect(x + 3, 31, 4, 1);
      g.springs.push_back({double(x + 4), 30.7, 1.3, .3});
      g.sweets.push_back({double(x + 16), 28.5});
    }
    if (variant == 3) {
      g.add_solid_rect(x + 15, 34, 6, 1);
      g.enemies.push_back({double(x + 15), double(x + 19), 33, 1.8});
      g.sweets.push_back({double(x + 18), 31});
    }
  }
  // Guaranteed examples in the last safe bay; none can block the exit.
  int end = 8 + rooms * span;
  g.add_solid_rect(end, 38, 16, 8);
  if (g.platforms.empty())
    g.platforms.push_back({{double(end + 2), 33, 4, .5}, 1, 0, 4, 0});
  if (g.springs.empty())
    g.springs.push_back({double(end + 1), 37.7, 1.3, .3});
  if (g.enemies.empty()) {
    g.add_solid_rect(end + 3, 32, 5, 1);
    g.enemies.push_back({double(end + 3), double(end + 6), 31, 1.5});
  }
  return g;
}

// Six authored precision rooms, with safe retries before every crossing.
EngineGrid build_summit() {
  EngineGrid g(216, 46);
  g.event_level = false;
  g.summit = true;
  g.spawn_x = 4;
  g.spawn_y = 38 - kPlayerHeight;
  g.goal = {208, 33, 3, 5};
  g.checkpoints = {4};
  g.add_solid_rect(0, 38, 8, 8);
  for (int room = 0; room < 6; ++room) {
    int x = 8 + room * 32;
    g.add_solid_rect(x, 38, 20, 8);
    g.checkpoints.push_back(x + 2);
    g.add_solid_rect(x + 7, 36 - room % 3, 3, 2 + room % 3);
    if (room >= 3)
      g.add_hazard_rect(x + 14, 37, 2);
    g.refills.push_back({double(x + 24), 34.5});
    g.sweets.push_back({double(x + 24), 31});
    // Optional upper wall-jump route and a spring-fed aerial collectible.
    if (room % 2 == 0) {
      g.add_solid_rect(x + 12, 29, 2, 5);
      g.sweets.push_back({double(x + 12), 26});
    } else {
      g.springs.push_back({double(x + 3), 37.7, 1.3, .3});
      g.platforms.push_back({{double(x + 15), 29, 3, .5}, 2, 0, 4, 0});
      g.sweets.push_back({double(x + 16), 26});
    }
  }
  g.add_solid_rect(200, 38, 16, 8);
  return g;
}

GameState new_state(const EngineGrid &grid) {
  GameState state;
  state.collected.resize(grid.sweets.size());
  state.defeated.resize(grid.enemies.size());
  state.refill_cooldown.resize(grid.refills.size());
  state.spawn_x = grid.spawn_x;
  state.spawn_y = grid.spawn_y;
  state.player.x = grid.spawn_x;
  state.player.y = grid.spawn_y;
  return state;
}

void reset_player(GameState &state) {
  state.player.x = state.spawn_x;
  state.player.y = state.spawn_y;
  state.player.vx = 0.0;
  state.player.vy = 0.0;
  state.player.grounded = false;
  state.player.dash_available = true;
  state.player.coyote_left = 0.0;
  state.player.jump_buffer_left = 0.0;
  state.player.dash_buffer_left = 0.0;
  state.player.dash_time_left = 0.0;
  state.player.wall_lock_left = 0.0;
  state.player.wall_coyote_left = 0;
  state.player.last_wall = 0;
  std::fill(state.refill_cooldown.begin(), state.refill_cooldown.end(), 0);
  state.player.support = -1;
  state.player.drop_left = 0;
  state.won = false;
}

void move_axis(const EngineGrid &grid, Player &player, double dt, char axis) {
  if (axis == 'x') {
    player.x += player.vx * dt;
  } else {
    player.y += player.vy * dt;
    player.grounded = false;
  }

  Rect rect = player.rect();
  for (const Rect &solid : grid.solid_rects_near(rect)) {
    if (!rect.intersects(solid)) {
      continue;
    }
    if (axis == 'x') {
      if (player.vx > 0.0) {
        player.x = solid.left() - rect.w;
      } else if (player.vx < 0.0) {
        player.x = solid.right();
      }
      player.vx = 0.0;
    } else {
      if (player.vy > 0.0) {
        player.y = solid.top() - rect.h;
        player.grounded = true;
        player.coyote_left = kCoyoteTime;
      } else if (player.vy < 0.0) {
        player.y = solid.bottom();
      }
      player.vy = 0.0;
    }
    rect = player.rect();
  }
}

int wall_dir(const EngineGrid &grid, const Player &player) {
  const Rect rect = player.rect();
  const Rect left_probe{rect.x - 0.08, rect.y + 0.25, rect.w, rect.h - 0.5};
  for (const Rect &solid : grid.solid_rects_near(left_probe)) {
    if (left_probe.intersects(solid)) {
      return -1;
    }
  }
  const Rect right_probe{rect.x + 0.08, rect.y + 0.25, rect.w, rect.h - 0.5};
  for (const Rect &solid : grid.solid_rects_near(right_probe)) {
    if (right_probe.intersects(solid)) {
      return 1;
    }
  }
  return 0;
}

void update_step(GameState &state, const EngineGrid &grid,
                 const Controls &controls, double dt) {
  Player &player = state.player;
  if (controls.restart) {
    reset_player(state);
    return;
  }

  player.wall_lock_left = std::max(0.0, player.wall_lock_left - dt);
  player.jump_buffer_left = std::max(0.0, player.jump_buffer_left - dt);
  player.dash_buffer_left = std::max(0.0, player.dash_buffer_left - dt);
  player.coyote_left = std::max(0.0, player.coyote_left - dt);
  if (controls.jump_pressed) {
    player.jump_buffer_left = kJumpBuffer;
  }
  if (controls.dash_pressed) {
    player.dash_buffer_left = kDashBuffer;
  }

  if (player.grounded) {
    player.dash_available = true;
  }

  if (player.dash_buffer_left > 0.0 && player.dash_available) {
    double dx = static_cast<double>(controls.move);
    double dy = static_cast<double>(controls.aim_y);
    if (dx == 0.0 && dy == 0.0) {
      dx = static_cast<double>(player.facing);
    }
    const double len = std::hypot(dx, dy);
    dx /= len;
    dy /= len;
    player.vx = dx * kDashSpeed;
    player.vy = dy * kDashSpeed;
    player.dash_time_left = kDashTime;
    player.dash_available = false;
    player.grounded = false;
    player.coyote_left = 0.0;
    player.wall_coyote_left = 0;
    player.jump_buffer_left = 0.0;
    player.dash_buffer_left = 0.0;
  }

  if (player.dash_time_left > 0.0) {
    player.dash_time_left = std::max(0.0, player.dash_time_left - dt);
    move_axis(grid, player, dt, 'x');
    move_axis(grid, player, dt, 'y');
    if (player.grounded) {
      player.dash_available = true;
    }
    player.x =
        clamp(player.x, 0.0,
              std::max(0.0, static_cast<double>(grid.width()) - kPlayerWidth));
    state.best_speed =
        std::max(state.best_speed, std::hypot(player.vx, player.vy));

    const Rect rect = player.rect();
    bool hit_hazard = false;
    for (const Rect &hazard : grid.hazard_rects_near(rect)) {
      if (rect.intersects(hazard)) {
        hit_hazard = true;
        break;
      }
    }
    if (rect.top() > grid.height() + 5.0 || hit_hazard) {
      ++state.deaths;
      reset_player(state);
      return;
    }
    if (grid.event_level ? rect.intersects(grid.goal)
                         : rect.right() >= grid.goal.x) {
      state.won = true;
    }
    return;
  }

  const int touching_wall = player.grounded ? 0 : wall_dir(grid, player);
  player.wall_coyote_left = std::max(0., player.wall_coyote_left - dt);
  if (player.grounded)
    player.wall_coyote_left = 0;
  else if (touching_wall) {
    player.last_wall = touching_wall;
    player.wall_coyote_left = .10;
  }
  if (touching_wall != 0 && controls.move == touching_wall &&
      player.vy > kWallSlideSpeed) {
    player.vy = kWallSlideSpeed;
  }

  if (player.jump_buffer_left > 0.0) {
    if (player.grounded || player.coyote_left > 0.0) {
      player.vy = -kJumpSpeed;
      player.grounded = false;
      player.coyote_left = 0.0;
      player.jump_buffer_left = 0.0;
    } else if (player.wall_coyote_left > 0) {
      player.vx = -player.last_wall * kWallJumpX;
      player.vy = -kWallJumpY;
      player.facing = -player.last_wall;
      player.wall_coyote_left = 0;
      player.wall_lock_left = 0.13;
      player.jump_buffer_left = 0.0;
    }
  }

  if (controls.move != 0 && player.wall_lock_left <= 0.0) {
    player.facing = controls.move;
    const double apex_bonus =
        (std::abs(player.vy) < 3.0 && !player.grounded) ? kApexAccelBonus : 0.0;
    const double accel = player.grounded ? kMoveAccel : kAirAccel + apex_bonus;
    const double target = controls.move * kMaxRunSpeed;
    player.vx += clamp(target - player.vx, -accel * dt, accel * dt);
  } else if (player.grounded) {
    player.vx = approach_zero(player.vx, kGroundFriction * dt);
  } else if (controls.move == 0 && player.wall_lock_left <= 0.0) {
    player.vx = approach_zero(player.vx, 24.0 * dt);
  }

  double gravity = kGravity;
  if (std::abs(player.vy) < 2.8 && !player.grounded) {
    gravity *= kApexGravityScale;
  }
  if (player.vy < -5.0 && !controls.jump_held) {
    gravity += kJumpCutGravity;
  }
  if (controls.fast_fall && player.vy > 0.0)
    gravity *= 1.65;
  player.vy += gravity * dt;
  const double fall_cap =
      (controls.fast_fall && player.vy > 0.0) ? kFastFallSpeed : kMaxFallSpeed;
  player.vy = std::min(player.vy, fall_cap);

  move_axis(grid, player, dt, 'x');
  move_axis(grid, player, dt, 'y');
  if (player.grounded) {
    player.dash_available = true;
  }
  player.x =
      clamp(player.x, 0.0,
            std::max(0.0, static_cast<double>(grid.width()) - kPlayerWidth));
  state.best_speed =
      std::max(state.best_speed, std::hypot(player.vx, player.vy));

  const Rect rect = player.rect();
  bool hit_hazard = false;
  for (const Rect &hazard : grid.hazard_rects_near(rect)) {
    if (rect.intersects(hazard)) {
      hit_hazard = true;
      break;
    }
  }
  if (rect.top() > grid.height() + 5.0 || hit_hazard) {
    ++state.deaths;
    reset_player(state);
    return;
  }
  if (grid.event_level ? rect.intersects(grid.goal)
                       : rect.right() >= grid.goal.x) {
    state.won = true;
  }
}

// Dynamic surfaces are one-way: carry riders, allow upward passage and S-drop.
void world_step(GameState &s, const EngineGrid &g, Controls c, double dt) {
  auto &p = s.player;
  double before_time = s.elapsed;
  int deaths = s.deaths;
  p.drop_left = std::max(0.0, p.drop_left - dt);
  if (p.support >= 0 && size_t(p.support) < g.platforms.size()) {
    auto old = g.platforms[p.support].at(before_time),
         now = g.platforms[p.support].at(before_time + dt);
    if (c.fast_fall) {
      p.drop_left = .22;
      p.grounded = false;
      p.support = -1;
    } else {
      p.x += now.x - old.x;
      p.y += now.y - old.y;
    }
  }
  Rect old = p.rect();
  bool dashing = p.dash_time_left > 0;
  s.elapsed += dt;
  s.notice = std::max(0.0, s.notice - dt);
  update_step(s, g, c, dt);
  p.support = -1;
  if (s.deaths != deaths || c.restart)
    return;
  if (p.drop_left <= 0 && p.vy >= 0) {
    for (size_t i = 0; i < g.platforms.size(); ++i) {
      auto r = g.platforms[i].at(s.elapsed),
           prev = g.platforms[i].at(before_time);
      if (old.bottom() <= prev.top() + .10 && p.rect().bottom() >= r.top() &&
          p.rect().right() > r.left() && p.x < r.right()) {
        p.y = r.y - kPlayerHeight;
        p.vy = 0;
        p.grounded = true;
        p.dash_available = true;
        p.coyote_left = kCoyoteTime;
        p.support = int(i);
      }
    }
  }
  for (size_t i = 0; i < g.refills.size(); ++i) {
    s.refill_cooldown[i] = std::max(0., s.refill_cooldown[i] - dt);
    auto [x, y] = g.refills[i];
    if (s.refill_cooldown[i] == 0 && !p.dash_available &&
        p.rect().intersects({x - .5, y - .5, 1, 1})) {
      p.dash_available = true;
      s.refill_cooldown[i] = 2.5;
    }
  }
  for (auto spring : g.springs)
    if (p.vy >= 0 && p.rect().intersects(spring)) {
      p.vy = -31;
      p.grounded = false;
      p.coyote_left = 0;
      p.dash_available = true;
      p.support = -1;
    }
  for (size_t i = 0; i < g.enemies.size(); ++i)
    if (!s.defeated[i]) {
      auto enemy = g.enemies[i].at(s.elapsed);
      if (p.rect().intersects(enemy)) {
        if (dashing || p.dash_time_left > 0 ||
            (old.bottom() <= enemy.top() + .3 && p.vy > 0)) {
          s.defeated[i] = true;
          p.dash_available = true;
          if (!dashing && p.dash_time_left <= 0) {
            p.vy = -17;
            p.grounded = false;
          }
        } else {
          ++s.deaths;
          reset_player(s);
          return;
        }
      }
    }
  // A carried player must never remain embedded in static terrain.
  for (auto solid : g.solid_rects_near(p.rect()))
    if (p.rect().intersects(solid)) {
      ++s.deaths;
      reset_player(s);
      return;
    }
}

// Shared 120 Hz simulation for both frontends. Events persist until a tick
// consumes them.
class FixedStepper {
public:
  double accumulator = 0.0;
  Controls pending{};
  void advance(GameState &state, const EngineGrid &grid, Controls input,
               double elapsed) {
    pending.jump_pressed |= input.jump_pressed;
    pending.dash_pressed |= input.dash_pressed;
    pending.restart |= input.restart;
    accumulator += clamp(elapsed, 0.0, 0.1);
    while (accumulator + 1e-10 >= 1.0 / 120.0) {
      input.jump_pressed = pending.jump_pressed;
      input.dash_pressed = pending.dash_pressed;
      input.restart = pending.restart;
      pending = Controls{};
      if (!state.won || input.restart) {
        world_step(state, grid, input, 1.0 / 120.0);
        for (size_t i = 0; i < grid.sweets.size(); ++i) {
          auto [x, y] = grid.sweets[i];
          if (!state.collected[i] &&
              state.player.rect().intersects({x - .35, y - .35, 1.1, 1.1})) {
            state.collected[i] = true;
            state.player.dash_available = true;
          }
        }
      }
      // Checkpoints only activate on safe ground, never while flying over a
      // gap.
      if (state.player.grounded) {
        for (size_t cp = 1; cp < grid.checkpoints.size(); ++cp) {
          double x = grid.checkpoints[cp];
          if (int(cp) > state.checkpoint && state.player.x >= x - 1 &&
              state.player.x < x + 5) {
            state.checkpoint = int(cp);
            state.notice = 5;
            state.spawn_x = x;
            state.spawn_y = 38 - kPlayerHeight;
          }
        }
      }

      accumulator -= 1.0 / 120.0;
    }
  }
};

// Compatibility helper for existing smoke probes; collision steps never exceed
// 1/120 s.
void update(GameState &state, const EngineGrid &grid, Controls input,
            double dt) {
  while (dt > 1e-9) {
    double step = std::min(dt, 1.0 / 120.0);
    update_step(state, grid, input, step);
    input.jump_pressed = input.dash_pressed = input.restart = false;
    dt -= step;
  }
}

void update_camera(GameState &state, const EngineGrid &grid, int view_width,
                   int play_height, double aspect, double dt) {
  const double visible_w = view_width * kWorldUnitsPerRenderCol;
  const double visible_h = play_height * aspect * kWorldUnitsPerRenderCol;
  double target_x = state.player.x - visible_w * 0.42 + state.player.vx * 0.15;
  double target_y = state.player.y - visible_h * 0.55 + state.player.vy * 0.065;
  target_x = clamp(target_x, 0.0, std::max(0.0, grid.width() - visible_w));
  target_y = clamp(target_y, 0.0, std::max(0.0, grid.height() - visible_h));
  const double blend = 1.0 - std::exp(-dt * 10.5);
  state.camera_x += (target_x - state.camera_x) * blend;
  state.camera_y += (target_y - state.camera_y) * blend;
}

struct TermSize {
  int rows = 24;
  int cols = 80;
  int x_pixels = 0;
  int y_pixels = 0;
};

TermSize term_size() {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO info{};
  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
    return {info.srWindow.Bottom - info.srWindow.Top + 1,
            info.srWindow.Right - info.srWindow.Left + 1, 0, 0};
#else
  winsize ws{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 &&
      ws.ws_col > 0) {
    return TermSize{static_cast<int>(ws.ws_row), static_cast<int>(ws.ws_col),
                    static_cast<int>(ws.ws_xpixel),
                    static_cast<int>(ws.ws_ypixel)};
  }
#endif
  return {};
}

[[maybe_unused]] double detect_cell_aspect() {
  const TermSize size = term_size();
  if (size.rows <= 0 || size.cols <= 0 || size.x_pixels <= 0 ||
      size.y_pixels <= 0) {
    return kDefaultCellAspect;
  }
  const double cell_w = static_cast<double>(size.x_pixels) / size.cols;
  const double cell_h = static_cast<double>(size.y_pixels) / size.rows;
  return clamp(cell_h / cell_w, 1.1, 3.4);
}

volatile std::sig_atomic_t stop_requested = 0;
void request_stop(int) { stop_requested = 1; }

#include "ssh_input.hpp"

#ifndef _WIN32
class TerminalGuard {
public:
  explicit TerminalGuard(bool ssh = false) : ssh_(ssh) {
    active_ = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    if (!active_) {
      return;
    }
    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);
    std::signal(SIGHUP, request_stop);
    tcgetattr(STDIN_FILENO, &original_);
    termios raw = original_;
    raw.c_lflag &= static_cast<unsigned>(~(ECHO | ICANON | IEXTEN | ISIG));
    raw.c_iflag &=
        static_cast<unsigned>(~(IXON | ICRNL | BRKINT | INPCK | ISTRIP));
    raw.c_oflag &= static_cast<unsigned>(~(OPOST));
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    std::cout << "\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[?1004h\x1b[2J\x1b[H"
              << std::flush;
    if (ssh_)
      std::cout << "\x1b[>11u\x1b[?u" << std::flush;
  }

  TerminalGuard(const TerminalGuard &) = delete;
  TerminalGuard &operator=(const TerminalGuard &) = delete;

  ~TerminalGuard() {
    if (!active_) {
      return;
    }
    if (ssh_)
      std::cout << "\x1b[<u";
    std::cout << "\x1b[?2026l\x1b[0m\x1b[?1004l\x1b[?7h\x1b[?25h\x1b[?1049l"
              << std::flush;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
  }

  bool active() const { return active_; }

private:
  bool active_ = false;
  termios original_{};
  bool ssh_ = false;
};

void read_input(InputState &input) {
  input.begin_frame();
  char buffer[128];
  const ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
  if (n <= 0) {
    return;
  }

  for (ssize_t i = 0; i < n; ++i) {
    unsigned char ch = static_cast<unsigned char>(buffer[i]);
    if (ch == 27 && i + 2 < n && buffer[i + 1] == '[') {
      const char code = buffer[i + 2];
      if (code == 'A')
        input.press('w');
      if (code == 'B')
        input.press('s');
      if (code == 'C')
        input.press('d');
      if (code == 'D')
        input.press('a');
      i += 2;
      continue;
    }
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<unsigned char>(ch - 'A' + 'a');
    }
    input.press(static_cast<char>(ch));
  }
}

class EvdevInput {
public:
  EvdevInput() {
    if (const char *inherited = std::getenv("TV_INPUT_FDS")) {
      std::istringstream list(inherited);
      std::string item;
      while (std::getline(list, item, ',')) {
        char *end = nullptr;
        long fd = std::strtol(item.c_str(), &end, 10);
        if (!*end && fd >= 3 && fd <= INT_MAX && looks_like_keyboard(int(fd))) {
          fcntl(int(fd), F_SETFL, fcntl(int(fd), F_GETFL) | O_NONBLOCK);
          fcntl(int(fd), F_SETFD, FD_CLOEXEC);
          fds_.push_back(int(fd));
          device_keys_.emplace_back();
          diagnostics_.push_back("session keyboard fd " + item +
                                 ": opened; game uid " +
                                 std::to_string(geteuid()));
        }
      }
      if (active())
        return;
    }
    open_matching("/dev/input/by-path");
    open_matching("/dev/input/by-id");
    if (fds_.empty()) {
      open_event_nodes("/dev/input");
    }
  }

  EvdevInput(const EvdevInput &) = delete;
  EvdevInput &operator=(const EvdevInput &) = delete;

  ~EvdevInput() {
    for (int fd : fds_) {
      close(fd);
    }
  }

  bool active() const { return !fds_.empty(); }

  std::string label() const { return active() ? "evdev" : "terminal"; }

  size_t device_count() const { return fds_.size(); }

  const std::vector<std::string> &diagnostics() const { return diagnostics_; }

  HeldKeys poll() {
    pressed_.fill(false);
    input_event event{};
    for (size_t device = 0; device < fds_.size(); ++device) {
      const int fd = fds_[device];
      while (true) {
        const ssize_t n = read(fd, &event, sizeof(event));
        if (n == static_cast<ssize_t>(sizeof(event))) {
          if (event.type == EV_SYN && event.code == SYN_DROPPED) {
            dropped_[fd] = true;
            device_keys_[device].fill(false);
            continue;
          }
          if (dropped_[fd]) {
            if (event.type == EV_SYN && event.code == SYN_REPORT) {
              resync(device);
              dropped_[fd] = false;
            }
            continue;
          }
          if (event.type == EV_KEY && event.code < KEY_CNT) {
            if (event.value == 1) {
              pressed_[event.code] = pressed_[event.code] || !key(event.code);
              device_keys_[device][event.code] = true;
            } else if (event.value == 0) {
              device_keys_[device][event.code] = false;
            }
          }
          continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
          break;
        }
        if (n == 0 || (n < 0 && errno == ENODEV))
          device_keys_[device].fill(false);
        break;
      }
    }

    return HeldKeys{
        key(KEY_A) || key(KEY_LEFT),
        key(KEY_D) || key(KEY_RIGHT),
        key(KEY_W) || key(KEY_UP),
        key(KEY_S) || key(KEY_DOWN),
        key(KEY_W) || key(KEY_UP) || key(KEY_SPACE),
        press(KEY_W) || press(KEY_UP) || press(KEY_SPACE),
        key(KEY_X) || key(KEY_K) || key(KEY_LEFTSHIFT) || key(KEY_RIGHTSHIFT),
        press(KEY_X) || press(KEY_K) || press(KEY_LEFTSHIFT) ||
            press(KEY_RIGHTSHIFT),
        press(KEY_R),
        press(KEY_Q),
    };
  }

  void ui_controls(Controls &c) const {
    c.select_level = press(KEY_1) ? 1 : press(KEY_2) ? 2 : press(KEY_3) ? 3 : 0;
    c.next_seed = press(KEY_G);
    c.menu_request = press(KEY_M);
    c.pause = press(KEY_ESC) || press(KEY_P);
    c.journal = press(KEY_TAB);
    c.confirm = press(KEY_ENTER);
    c.debug = press(KEY_F3);
    c.new_run = press(KEY_N) ||
                (press(KEY_R) && (key(KEY_LEFTCTRL) || key(KEY_RIGHTCTRL)));
  }

private:
  void resync(size_t device) {
    std::array<unsigned char, (KEY_CNT + 7) / 8> bits{};
    if (ioctl(fds_[device], EVIOCGKEY(bits.size()), bits.data()) >= 0)
      for (int key = 0; key < KEY_CNT; ++key)
        device_keys_[device][key] = (bits[key / 8] >> (key % 8)) & 1;
  }
  std::map<int, bool> dropped_;
  bool key(unsigned code) const {
    if (code >= KEY_CNT) {
      return false;
    }
    for (const auto &keys : device_keys_) {
      if (keys[code]) {
        return true;
      }
    }
    return false;
  }

  bool press(unsigned code) const {
    return code < pressed_.size() && pressed_[code];
  }

  void open_matching(const std::filesystem::path &dir) {
    std::error_code error;
    if (!std::filesystem::exists(dir, error)) {
      diagnostics_.push_back(dir.string() + ": missing");
      return;
    }
    for (const auto &entry : std::filesystem::directory_iterator(dir, error)) {
      if (error) {
        diagnostics_.push_back(dir.string() +
                               ": cannot list: " + error.message());
        return;
      }
      const std::string name = entry.path().filename().string();
      if (name.find("kbd") == std::string::npos &&
          name.find("keyboard") == std::string::npos) {
        continue;
      }
      const int fd =
          open(entry.path().c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
      if (fd >= 0) {
        struct stat current{};
        fstat(fd, &current);
        bool duplicate = false;
        for (int existing : fds_) {
          struct stat other{};
          fstat(existing, &other);
          if (other.st_rdev == current.st_rdev)
            duplicate = true;
        }
        if (duplicate || !looks_like_keyboard(fd)) {
          close(fd);
          continue;
        }
        fds_.push_back(fd);
        device_keys_.emplace_back();
        diagnostics_.push_back(entry.path().string() + ": opened");
      } else {
        diagnostics_.push_back(entry.path().string() +
                               ": open failed: " + std::strerror(errno));
      }
    }
  }

  bool has_bit(const std::vector<unsigned long> &bits, unsigned code) const {
    const unsigned word = code / (sizeof(unsigned long) * CHAR_BIT);
    const unsigned bit = code % (sizeof(unsigned long) * CHAR_BIT);
    return word < bits.size() && ((bits[word] >> bit) & 1UL);
  }

  bool looks_like_keyboard(int fd) const {
    std::vector<unsigned long> key_bits(
        (KEY_CNT + sizeof(unsigned long) * CHAR_BIT - 1) /
        (sizeof(unsigned long) * CHAR_BIT));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, key_bits.size() * sizeof(unsigned long)),
              key_bits.data()) < 0) {
      return false;
    }
    return has_bit(key_bits, KEY_A) && has_bit(key_bits, KEY_D) &&
           has_bit(key_bits, KEY_W) && has_bit(key_bits, KEY_SPACE);
  }

  void open_event_nodes(const std::filesystem::path &dir) {
    std::error_code error;
    if (!std::filesystem::exists(dir, error)) {
      diagnostics_.push_back(dir.string() + ": missing");
      return;
    }
    for (const auto &entry : std::filesystem::directory_iterator(dir, error)) {
      if (error) {
        diagnostics_.push_back(dir.string() +
                               ": cannot list: " + error.message());
        return;
      }
      const std::string name = entry.path().filename().string();
      if (name.rfind("event", 0) != 0) {
        continue;
      }
      const int fd =
          open(entry.path().c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
      if (fd < 0) {
        diagnostics_.push_back(entry.path().string() +
                               ": open failed: " + std::strerror(errno));
        continue;
      }
      if (looks_like_keyboard(fd)) {
        fds_.push_back(fd);
        device_keys_.emplace_back();
        diagnostics_.push_back(entry.path().string() + ": opened keyboard");
      } else {
        diagnostics_.push_back(entry.path().string() +
                               ": readable but not a keyboard");
        close(fd);
      }
    }
  }

  std::vector<int> fds_;
  std::vector<std::array<bool, KEY_CNT>> device_keys_;
  std::array<bool, KEY_CNT> pressed_{};
  std::vector<std::string> diagnostics_;
};

class TuiInput {
public:
  explicit TuiInput(InputMode mode)
      : mode_(mode), ssh_(mode == InputMode::SshBasic) {
    if (mode != InputMode::Ssh && mode != InputMode::SshBasic)
      evdev_ = std::make_unique<EvdevInput>();
  }

  bool ready() const {
    return mode_ != InputMode::Evdev || (evdev_ && evdev_->active());
  }

  bool focused = true;
  Controls poll(double dt) {
    if (mode_ == InputMode::Ssh || mode_ == InputMode::SshBasic) {
      auto c = ssh_.poll(dt);
      focused = ssh_.focused;
      return c;
    }
    if (mode_ != InputMode::Terminal && evdev_ && evdev_->active()) {
      // Movement comes from evdev. The terminal stream is drained only for
      // focus notifications.
      char bytes[1024];
      ssize_t n;
      while ((n = read(STDIN_FILENO, bytes, sizeof(bytes))) > 0)
        focus_buffer_.append(bytes, size_t(n));
      for (size_t i = 0; i + 2 < focus_buffer_.size(); ++i) {
        if (focus_buffer_.compare(i, 3, "\x1b[O") == 0)
          focused = false;
        if (focus_buffer_.compare(i, 3, "\x1b[I") == 0)
          focused = true;
      }
      if (focus_buffer_.size() > 2)
        focus_buffer_ = focus_buffer_.substr(focus_buffer_.size() - 2);
      Controls c = evdev_->poll().controls();
      evdev_->ui_controls(c);
      return c;
    }
    read_input(terminal_);
    Controls controls = terminal_.controls();
    controls.confirm = terminal_.jump_pressed;
    terminal_.decay(dt);
    return controls;
  }

  bool basic_ssh() const {
    return (mode_ == InputMode::Ssh || mode_ == InputMode::SshBasic) &&
           !ssh_.enhanced;
  }
  void clear() { ssh_.clear(); }
  std::string label() const {
    if (mode_ == InputMode::Ssh || mode_ == InputMode::SshBasic)
      return ssh_.enhanced ? "SSH • press/release" : "SSH • A/D bieg, S stop";
    if (mode_ == InputMode::Terminal) {
      return "terminal";
    }
    if (evdev_ && evdev_->active()) {
      return "evdev";
    }
    return "missing-evdev";
  }

private:
  InputMode mode_ = InputMode::Evdev;
  std::unique_ptr<EvdevInput> evdev_;
  SshInput ssh_;
  InputState terminal_;
  std::string focus_buffer_;
};

#else
#include "windows_console.hpp"
#endif

enum class Color {
  Gold,
  Ghost,
  Decor,
  Default,
  Solid,
  Hazard,
  Goal,
  Player,
  Hud,
  Text,
};

struct Cell {
  std::string glyph = " ";
  Color color = Color::Default;
  bool reverse = false;
  bool bold = false;
  unsigned braille_mask = 0;
};

std::string utf8(char32_t value) {
  std::string out;
  if (value <= 0x7F) {
    out.push_back(static_cast<char>(value));
  } else if (value <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | ((value >> 6) & 0x1F)));
    out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
  } else if (value <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | ((value >> 12) & 0x0F)));
    out.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
  }
  return out;
}

unsigned braille_dot(int x, int y) {
  static constexpr unsigned dots[4][2] = {
      {0x01, 0x08},
      {0x02, 0x10},
      {0x04, 0x20},
      {0x40, 0x80},
  };
  return dots[y][x];
}

Cell braille_cell(unsigned mask, Color color, bool bold = false) {
  if (mask == 0) {
    return Cell{};
  }
  return Cell{utf8(0x2800 + mask), color, false, bold, mask};
}

std::string ansi_for(Color c, bool, bool bold) {
  std::string out = "\x1b[0m\x1b[48;2;25;20;24m";
  if (bold)
    out += "\x1b[1m";
  switch (c) {
  case Color::Gold:
    out += "\x1b[38;2;244;205;94m";
    break;
  case Color::Ghost:
    out += "\x1b[38;2;93;201;210m";
    break;
  case Color::Decor:
    out += "\x1b[38;2;65;42;49m";
    break;
  case Color::Solid:
    out += "\x1b[38;2;126;65;77m";
    break;
  case Color::Hazard:
    out += "\x1b[38;2;235;57;79m";
    break;
  case Color::Goal:
    out += "\x1b[38;2;244;205;94m";
    break;
  case Color::Player:
    out += "\x1b[38;2;250;247;239m";
    break;
  default:
    out += "\x1b[38;2;200;187;184m";
    break;
  }
  return out;
}

class RenderGrid {
public:
  RenderGrid(const EngineGrid &grid, const GameState &state, int rows, int cols,
             double aspect)
      : grid_(grid), state_(state), rows_(std::max(1, rows)),
        cols_(std::max(1, cols)), aspect_(aspect),
        play_rows_(std::max(1, rows - 7)) {
    buffer_.assign(rows_, std::vector<Cell>(cols_));
  }
  void draw() {
    if (rows_ < 24 || cols_ < 76) {
      text(1, 2, "Powiększ terminal do co najmniej 76 × 24.");
      text(3, 2, "Zalecane: 110 × 34. Q — wyjście.");
      flush();
      return;
    }
    for (int y = 0; y < play_rows_; ++y)
      for (int x = 0; x < cols_; ++x)
        buffer_[y + 4][x] = world_cell(x, y);
    for (size_t i = 0; i < grid_.sweets.size(); ++i)
      if (!state_.collected[i]) {
        auto [x, y] = screen(grid_.sweets[i].first, grid_.sweets[i].second);
        text(y, x, "▰", Color::Gold);
      }
    for (size_t i = 0; i < grid_.checkpoints.size(); ++i) {
      double wx = grid_.checkpoints[i];
      auto [x, y] = screen(wx, 34.5);
      text(y, x, "│██", Color::Player);
      text(y + 1, x, "│██", Color::Hazard);
      text(y + 2, x, "│", Color::Gold);
    }
    for (auto platform : grid_.platforms) {
      auto r = platform.at(state_.elapsed);
      auto [x, y] = screen(r.x, r.y);
      text(y, x,
           std::string(std::max(1, int(r.w / kWorldUnitsPerRenderCol)), '='),
           Color::Gold);
      text(y - 1, x, "↔", Color::Gold);
    }
    for (auto spring : grid_.springs) {
      auto [x, y] = screen(spring.x, spring.y);
      text(y, x, "^^^", Color::Goal);
    }
    for (size_t i = 0; i < grid_.enemies.size(); ++i)
      if (!state_.defeated[i]) {
        auto r = grid_.enemies[i].at(state_.elapsed);
        auto [x, y] = screen(r.x, r.y);
        text(y, x, "[! ]", Color::Hazard);
      }
    auto [gx, gy] = screen(grid_.goal.x, grid_.goal.y - 1);
    text(gy, gx, "META", Color::Gold);
    if (grid_.event_level) {
      auto [px, py] = screen(state_.player.x - .15, state_.player.y - .9);
      text(py, px, "JP2", Color::Gold);
    }
    for (size_t i = 0; i < grid_.refills.size(); ++i) {
      auto [x, y] = screen(grid_.refills[i].first, grid_.refills[i].second);
      text(y, x, state_.refill_cooldown[i] > 0 ? "◇" : "◆",
           state_.refill_cooldown[i] > 0 ? Color::Decor : Color::Ghost);
    }
    fill(0, 0, cols_, 3, Color::Default);
    text(0, 0, "█", Color::Player);
    text(1, 0, "█", Color::Hazard);
    text(0, 2,
         grid_.event_level ? "EVENT / JP2: SZTAFETA WOLNOŚCI"
         : grid_.summit    ? "TERMINAL VELOCITY / SZCZYT"
                           : "TERMINAL VELOCITY / EKSPEDYCJA",
         Color::Gold);
    if (state_.multiplayer)
      text(0, cols_ - 12, "DUCHY " + std::to_string(state_.ghosts.size()),
           Color::Ghost);
    int chapter = state_.player.x < 60 ? 0 : state_.player.x < 118 ? 1 : 2;
    const char *names[] = {"01 WADOWICE", "02 KRAKÓW", "03 WARSZAWA"};
    text(1, 2,
         grid_.event_level ? std::string(names[chapter]) +
                                 "   11 LISTOPADA • NIEPODLEGŁOŚĆ POLSKI"
         : grid_.summit
             ? "6 ETAPÓW   •   ŚCIANY / PRZEPAŚCIE / ODNOWIENIA ZRYWU"
             : "SEED " + std::to_string(grid_.seed) +
                   "   •   WINDY / SPRĘŻYNY / PATROLE",
         Color::Player);
    text(2, 2,
         (grid_.event_level ? "Kremówki " : "Kryształy ") +
             std::to_string(state_.sweets_count()) + "/" +
             std::to_string(grid_.sweets.size()) + "  |  Zryw: " +
             (state_.player.dash_available ? "GOTOWY" : "wyląduj") +
             "  |  Czas " + fixed(state_.elapsed) + " s  |  Próby " +
             std::to_string(state_.deaths + 1),
         Color::Text);
    for (int i = 0; i < cols_; ++i)
      put(3, i,
          {"━", i < state_.player.x / grid_.width() * cols_ ? Color::Hazard
                                                            : Color::Decor});
    fill(rows_ - 3, 0, cols_, 3, Color::Default);
    text(
        rows_ - 3, 2,
        state_.ssh_basic
            ? "SSH: A/D wybierz bieg • S zatrzymaj • SPACJA skok • X zryw"
            : "A D ruch   SPACJA skok   SHIFT/X zryw   S szybki spadek / zejdź",
        Color::Player);
    text(rows_ - 2, 2,
         "R checkpoint  N ponów  G nowy seed  M menu  TAB pomoc  P pauza  Q "
         "koniec",
         Color::Text);
    text(rows_ - 1, 2,
         "F3 debug  •  " + state_.input_source +
             (state_.ssh ? "  •  fizyka 120 Hz / obraz 30 Hz"
                         : "  •  fizyka 120 Hz / obraz 60 Hz"),
         Color::Gold);
    if (state_.notice > 0) {
      center(5, "PUNKT ZAPISU — R wraca tutaj", Color::Gold);
      center(6,
             !grid_.event_level
                 ? "Kolejny bezpieczny punkt. N powtarza ten sam seed."
             : state_.checkpoint == 1
                 ? "1918: odrodzenie Polski po 123 latach zaborów."
                 : "11 XI 1918: Piłsudski przejmuje władzę wojskową.",
             Color::Player);
    }
    if (state_.debug) {
      text(8, 2,
           "vx=" + fixed(state_.player.vx) + " vy=" + fixed(state_.player.vy) +
               " grunt=" + std::to_string(state_.player.grounded),
           Color::Gold);
      text(9, 2, "ciało 0.90 × 1.80 u  |  " + state_.input_source, Color::Text);
    }
    if (state_.journal)
      draw_journal();
    else if (state_.menu)
      draw_menu();
    else if (state_.paused)
      draw_pause();
    else if (state_.won)
      draw_win();
    flush();
  }

private:
  const EngineGrid &grid_;
  const GameState &state_;
  int rows_, cols_;
  double aspect_;
  int play_rows_;
  std::vector<std::vector<Cell>> buffer_;
  static std::vector<std::string> glyphs(const std::string &s) {
    std::vector<std::string> v;
    for (size_t i = 0; i < s.size();) {
      unsigned char c = s[i];
      size_t n = c < 128 ? 1 : c < 224 ? 2 : c < 240 ? 3 : 4;
      n = std::min(n, s.size() - i);
      v.push_back(s.substr(i, n));
      i += n;
    }
    return v;
  }
  static std::string fixed(double v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%.1f", v);
    return b;
  }
  void put(int row, int col, Cell c) {
    if (row >= 0 && row < rows_ && col >= 0 && col < cols_)
      buffer_[row][col] = std::move(c);
  }
  void fill(int row, int col, int w, int h, Color c) {
    for (int y = row; y < row + h; ++y)
      for (int x = col; x < col + w; ++x)
        put(y, x, {" ", c});
  }
  void text(int row, int col, const std::string &s, Color c = Color::Text) {
    for (auto ch : glyphs(s))
      put(row, col++, {ch, c, false, false});
  }
  void center(int row, const std::string &s, Color c = Color::Text) {
    text(row, (cols_ - int(glyphs(s).size())) / 2, s, c);
  }
  std::pair<int, int> screen(double x, double y) const {
    return {int(std::floor((x - state_.camera_x) / kWorldUnitsPerRenderCol)),
            4 + int(std::floor((y - state_.camera_y) /
                               (aspect_ * kWorldUnitsPerRenderCol)))};
  }
  Cell world_cell(int x, int y) const {
    double wx = state_.camera_x + x * kWorldUnitsPerRenderCol,
           wy = state_.camera_y + y * aspect_ * kWorldUnitsPerRenderCol;
    unsigned solid = 0, edge = 0, hazard = 0, body = 0, goal = 0, ghost = 0;
    for (int dy = 0; dy < 4; ++dy)
      for (int dx = 0; dx < 2; ++dx) {
        double a = wx + (dx + .5) * kWorldUnitsPerRenderCol / 2,
               b = wy + (dy + .5) * aspect_ * kWorldUnitsPerRenderCol / 4;
        int col = int(std::floor(a)), row = int(std::floor(b));
        unsigned dot = braille_dot(dx, dy);
        if (state_.player.rect().intersects({a, b, .01, .01}))
          body |= dot;
        for (const auto &g : state_.ghosts)
          if (Rect{g.x, g.y, kPlayerWidth, kPlayerHeight}.intersects(
                  {a, b, .01, .01}))
            ghost |= dot;
        if (grid_.solid_at(col, row)) {
          solid |= dot;
          if (!grid_.solid_at(col, row - 1) && b - row < .2)
            edge |= dot;
        }
        if (grid_.hazard_at(col, row) && b - row > .35)
          hazard |= dot;
        if (grid_.goal.intersects({a, b, .01, .01}))
          goal |= dot;
      }
    if (body)
      return braille_cell(
          body, state_.player.dash_available ? Color::Gold : Color::Player,
          true);
    if (hazard)
      return braille_cell(hazard, Color::Hazard, true);
    if (ghost && !solid)
      return braille_cell(ghost, Color::Ghost);
    if (goal)
      return braille_cell(goal, Color::Gold);
    if (edge)
      return braille_cell(solid, Color::Player);
    if (solid)
      return solid == 255 ? Cell{"█", Color::Solid}
                          : braille_cell(solid, Color::Solid);
    // Quiet architectural silhouettes and stars stay behind the collision
    // geometry.
    int skyline = 31 + (int(wx / 4) % 4);
    if (wy > skyline && wy < 42)
      return {((x + y) % 17 == 0) ? "▪" : "░", Color::Decor};
    if ((x * 47 + y * 71) % 173 == 0)
      return {"·", Color::Decor};
    return {};
  }
  void panel() { fill(4, 2, cols_ - 4, rows_ - 8, Color::Default); }
  void draw_menu() {
    panel();
    center(5, "T E R M I N A L   V E L O C I T Y", Color::Player);
    center(7,
           grid_.event_level ? "[1] EVENT: JP2 / 11 LISTOPADA  ◀"
                             : "[1] EVENT: JP2 / 11 LISTOPADA",
           Color::Gold);
    center(9,
           !grid_.event_level && !grid_.summit
               ? "[2] EKSPEDYCJA PROCEDURALNA  ◀"
               : "[2] EKSPEDYCJA PROCEDURALNA",
           Color::Gold);
    center(10,
           grid_.summit ? "[3] SZCZYT / PRECYZJA  ◀" : "[3] SZCZYT / PRECYZJA",
           Color::Gold);
    center(11, grid_.event_level
                   ? "Oryginalna trasa, kremówki i dziennik niepodległości."
                   : "Seed: " + std::to_string(grid_.seed) +
                         "   [G] losuj nowy / [N] ponów ten sam");
    center(13, "[ENTER] GRAJ     [TAB] HISTORIA / MECHANIKI", Color::Player);
    center(15, "Ruch i skoki jak wcześniej. Windy są jednostronne: S schodzi.");
    center(16, "◆ odnawia zryw i wraca po 2.5 s. Żółty gracz = zryw gotowy.");
    center(
        18,
        state_.ssh_basic
            ? "SSH podstawowe: A/D uruchamia bieg, S zatrzymuje, X robi zryw."
            : "SPACJA: skok o zmiennej wysokości. SHIFT/X + kierunek: zryw.");
    if (rows_ >= 29) {
      center(21, "R checkpoint  •  N powtórz  •  M menu  •  Q wyjście");
      center(23, "Stała fizyka 120 Hz. GUI pozostaje narzędziem debugowania.");
    }
  }
  void draw_journal() {
    panel();
    if (!grid_.event_level) {
      center(5, "EKSPEDYCJA / MECHANIKI", Color::Gold);
      center(8, "=== Ruchome platformy: wskakuj od dołu, S schodzi przez nie.");
      center(10, "^^ Sprężyny: automatyczny wysoki skok i odnowienie zrywu.");
      center(12, "[!] Patrole: skok na głowę albo zryw usuwa wroga. Bok boli.");
      center(14, "◆ odnawia zryw i wraca po 2.5 s. ▰ to jednorazowy skarb.");
      center(16, "Seed tworzy tę samą mapę. N powtarza, G losuje następną.");
      center(18, "TAB / ESC / ENTER — powrót", Color::Gold);
      return;
    }

    center(5, "DZIENNIK NIEPODLEGŁOŚCI", Color::Gold);
    const std::vector<std::pair<std::string, Color>> lines = {
        {"1795 → 1918 • 123 LATA ZABORÓW", Color::Gold},
        {"Trzeci rozbiór zakończył istnienie I Rzeczypospolitej.", Color::Text},
        {"Język, kultura i dążenie do wolności przetrwały przez pokolenia.",
         Color::Text},
        {"11 XI 1918 • ODRODZENIE PAŃSTWA", Color::Gold},
        {"Piłsudski przejął od Rady Regencyjnej władzę wojskową.", Color::Text},
        {"To symboliczna data. Odbudowa Polski była dłuższym procesem.",
         Color::Text},
        {"1920 / 1978 • JAN PAWEŁ II", Color::Gold},
        {"Karol Wojtyła urodził się w Wadowicach 18 maja 1920 roku.",
         Color::Text},
        {"Papieżem został 16 października 1978 r. To późniejsza epoka.",
         Color::Text},
        {"Źródła: Sejm RP i Watykan. Pełne odnośniki: README.md.",
         Color::Decor}};
    for (size_t i = 0; i < lines.size(); ++i)
      center(7 + int(i), lines[i].first, lines[i].second);
    center(18, "[ TAB / ESC / ENTER ] POWRÓT", Color::Gold);
  }
  void draw_pause() {
    panel();
    center(8, "CHWILA ODDECHU", Color::Gold);
    center(11, "[ ENTER / ESC ] WZNÓW");
    center(13, "[ TAB ] HISTORIA    [ N ] NOWY BIEG    [ Q ] WYJŚCIE");
    center(16, "Utrata fokusu Ghostty automatycznie pauzuje grę.");
  }
  void draw_win() {
    panel();
    center(7,
           grid_.event_level ? "N I E P O D L E G Ł A !"
                             : "E K S P E D Y C J A   U K O Ń C Z O N A",
           Color::Gold);
    center(10,
           grid_.event_level
               ? "Biało-czerwona dotarła do Warszawy."
               : "Seed " + std::to_string(grid_.seed) + " • dotarłeś do mety.",
           Color::Player);
    center(12, "Czas: " + fixed(state_.elapsed) +
                   " s    Znajdźki: " + std::to_string(state_.sweets_count()) +
                   "/" + std::to_string(grid_.sweets.size()) +
                   "    Próby: " + std::to_string(state_.deaths + 1));
    center(14,
           state_.sweets_count() == int(grid_.sweets.size())
               ? "KOMPLET ZNAJDZIEK!"
               : "Wyzwanie: zbierz wszystkie znajdźki.",
           Color::Gold);
    center(17, "[N] PONÓW   [G] NOWY SEED   [M] MENU   [Q] WYJŚCIE");
  }
  void flush() const {
    static std::vector<std::vector<Cell>> previous;
    bool full = previous.size() != buffer_.size() || previous.empty() ||
                previous[0].size() != buffer_[0].size();
    std::string out = "\x1b[?2026h";
    out.reserve(rows_ * cols_ * 4);
    if (full)
      out += "\x1b[2J";
    Color current = Color::Default;
    bool bold = false;
    out += ansi_for(current, false, false);
    for (int y = 0; y < rows_; ++y) {
      bool positioned = false;
      for (int x = 0; x < cols_; ++x) {
        const auto &c = buffer_[y][x];
        bool changed = full || c.glyph != previous[y][x].glyph ||
                       c.color != previous[y][x].color ||
                       c.bold != previous[y][x].bold;
        if (!changed) {
          positioned = false;
          continue;
        }
        if (!positioned) {
          out += "\x1b[" + std::to_string(y + 1) + ";" + std::to_string(x + 1) +
                 "H";
          positioned = true;
        }
        if (current != c.color || bold != c.bold) {
          out += ansi_for(c.color, false, c.bold);
          current = c.color;
          bold = c.bold;
        }
        out += c.glyph;
      }
    }
    out += "\x1b[0m\x1b[?2026l";
    std::cout << out << std::flush;
    previous = buffer_;
  }
};

[[maybe_unused]] int run_smoke(double aspect, int steps) {
  EngineGrid grid = build_level();
  GameState state = new_state(grid);
  InputState input;
  constexpr double dt = 1.0 / kTargetFps;

  for (int frame = 0; frame < steps; ++frame) {
    input.begin_frame();
    input.press('d');
    if (frame == 34) {
      input.press('x');
    }
    if (frame == 18 || frame == 62 || frame == 106 || frame == 151 ||
        frame == 196) {
      input.press(' ');
    }
    update(state, grid, input.controls(), dt);
    update_camera(state, grid, 90, 24, aspect, dt);
    input.decay(dt);
  }

  const Rect rect = state.player.rect();
  if (rect.left() < 0.0 || rect.right() > grid.width() + 1.0 ||
      std::abs(rect.h - kPlayerHeight) > 0.0001) {
    std::cerr << "smoke failed: invalid player rect\n";
    return 1;
  }

  std::cout << "smoke ok: x=" << rect.x << " y=" << rect.y
            << " vx=" << state.player.vx << " vy=" << state.player.vy
            << " grounded=" << (state.player.grounded ? "true" : "false")
            << " deaths=" << state.deaths << " dash_available="
            << (state.player.dash_available ? "true" : "false")
            << " engine_body=" << rect.h << " render_aspect=" << aspect << "\n";
  return 0;
}

[[maybe_unused]] int run_game(double aspect, InputMode input_mode,
                              bool demo = false, bool event_level = false,
                              uint32_t seed = 1, bool summit = false,
                              const std::string &ghost_host = "",
                              const std::string &ghost_port = "27888",
                              const std::string &room = "default") {
  bool remote =
      input_mode == InputMode::Ssh || input_mode == InputMode::SshBasic;
  TerminalGuard terminal(input_mode == InputMode::Ssh);
  if (!terminal.active()) {
#ifdef _WIN32
    std::cerr << "Run in Windows Terminal: play.cmd\n";
#else
    std::cerr << "Uruchom w Ghostty: ./play.sh\n";
#endif
    return 1;
  }
  ghosts::Client network;
  if (!ghost_host.empty())
    network.connect_to(ghost_host, ghost_port);
  uint32_t network_epoch = 0;
  int previous_deaths = 0;
  EngineGrid grid = summit        ? build_summit()
                    : event_level ? build_level()
                                  : build_procedural(seed);
  GameState state = new_state(grid);
  state.ssh = remote;
  TuiInput input(input_mode);
  if (!demo && !input.ready()) {
    std::cerr << "Brak dostępu do evdev. Użyj ./play.sh (pomocnik dostępu "
                 "tylko na czas gry).\n";
    return 1;
  }
  state.input_source = demo ? "DEMO — sterowanie automatyczne" : input.label();
  state.menu = !demo;
  FixedStepper stepper;
  auto last = std::chrono::steady_clock::now(), render_at = last;
  double end_wait = 0;
  auto size = term_size();
  update_camera(state, grid, size.cols, std::max(1, size.rows - 7), aspect, 10);
  while (!stop_requested) {
    auto now = std::chrono::steady_clock::now();
    double dt = clamp(std::chrono::duration<double>(now - last).count(), 0, .1);
    last = now;
    Controls c = demo ? Controls{} : input.poll(dt);
    if (demo) {
      auto &p = state.player;
      c.move = 1;
      c.jump_held = true;
      int foot = int(std::floor(p.y + kPlayerHeight + .05));
      bool trouble = false;
      for (double dx = 1; dx <= 2.5; dx += .5) {
        int x = int(p.x + kPlayerWidth + dx);
        trouble |= grid.solid_at(x, foot - 1) || grid.hazard_at(x, foot - 1) ||
                   !grid.solid_at(x, foot);
      }
      c.jump_pressed = p.grounded && trouble;
      int x = int(p.x + kPlayerWidth + 1);
      c.dash_pressed =
          !p.grounded && p.vy >= 0 && !grid.solid_at(x, 38) && p.dash_available;
      for (size_t e = 0; e < grid.enemies.size(); ++e)
        if (!state.defeated[e]) {
          auto enemy = grid.enemies[e].at(state.elapsed);
          if (enemy.x >= p.x - 1 && enemy.x - p.x < 2.5 &&
              std::abs(enemy.y - p.y) < 2 && p.dash_available)
            c.dash_pressed = true;
        }
      if (state.won) {
        end_wait += dt;
        if (end_wait > 3)
          return 0;
      }
    }
    if (!demo && !input.focused) {
      state.paused = true;
      c = {};
      stepper = FixedStepper{};
    }
    if (c.quit)
      return 0;
    state.ssh_basic = input.basic_ssh();
    if (!demo)
      state.input_source = input.label();
    if (c.menu_request) {
      state.menu = true;
      state.paused = false;
      state.journal = false;
      input.clear();
      stepper = FixedStepper{};
    }
    if ((c.select_level && (state.menu || state.paused || state.won)) ||
        c.next_seed) {
      if (c.next_seed) {
        event_level = false;
        summit = false;
        ++seed;
      } else {
        event_level = c.select_level == 1;
        summit = c.select_level == 3;
      }
      ++network_epoch;
      grid = summit        ? build_summit()
             : event_level ? build_level()
                           : build_procedural(seed);
      state = new_state(grid);
      state.ssh = remote;
      state.menu = true;
      state.ssh_basic = input.basic_ssh();
      state.input_source = input.label();
      input.clear();
      stepper = FixedStepper{};
      update_camera(state, grid, size.cols, std::max(1, size.rows - 7), aspect,
                    10);
    }
    if (c.debug)
      state.debug = !state.debug;
    if (c.journal) {
      state.journal = !state.journal;
      stepper = FixedStepper{};
    }
    if (c.pause) {
      if (state.journal)
        state.journal = false;
      else if (!state.menu && !state.won)
        state.paused = !state.paused;
      stepper = FixedStepper{};
    }
    if (c.new_run || (c.confirm && state.won && !state.journal)) {
      ++network_epoch;
      auto label = state.input_source;
      state = new_state(grid);
      state.ssh = remote;
      state.ssh_basic = input.basic_ssh();
      state.input_source = label;
      stepper = FixedStepper{};
      update_camera(state, grid, size.cols, std::max(1, size.rows - 7), aspect,
                    10);
    }
    if (c.confirm) {
      if (state.journal)
        state.journal = false;
      else {
        state.menu = false;
        state.paused = false;
      }
      c.jump_pressed = false;
      input.clear();
      stepper = FixedStepper{};
    }
    size = term_size();
    bool too_small = size.cols < 76 || size.rows < 24;
    if (!state.menu && !state.paused && !state.journal && !too_small)
      stepper.advance(state, grid, c, dt);
    else
      stepper = FixedStepper{};
    update_camera(state, grid, size.cols, std::max(1, size.rows - 7), aspect,
                  dt);
    if (c.restart || state.deaths != previous_deaths)
      ++network_epoch;
    previous_deaths = state.deaths;
    state.multiplayer = network.enabled();
    ghosts::Snapshot snapshot;
    snapshot.room = room;
    snapshot.level = grid.summit ? 2 : grid.event_level ? 0 : 1;
    snapshot.seed = grid.event_level || grid.summit ? 0 : grid.seed;
    snapshot.x = state.player.x;
    snapshot.y = state.player.y;
    snapshot.epoch = network_epoch;
    snapshot.flags =
        (state.player.dash_available ? 1 : 0) |
        (!state.menu && !state.paused && !state.journal && !too_small ? 2 : 0) |
        (state.won ? 4 : 0);
    network.tick(snapshot, dt);
    state.ghosts = network.visible();
    if (now >= render_at) {
      RenderGrid(grid, state, size.rows, size.cols, aspect).draw();
      render_at = now + std::chrono::microseconds(remote ? 33333 : 16667);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return 0;
}

struct Args {
  bool event_level = false, summit = false;
  int relay_port = 0;
  std::string ghost_host, ghost_port = "27888", room = "default";
  uint32_t seed =
      uint32_t(std::chrono::system_clock::now().time_since_epoch().count());
  bool demo = false;
  std::string preview;
  bool smoke = false;
  bool input_test = false;
  int smoke_steps = 240;
  double cell_aspect = 0.0;
  InputMode input_mode = InputMode::Evdev;
};

[[maybe_unused]] Args parse_args(int argc, char **argv,
                                 bool debug_frontend = false) {
  Args args;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--event") {
      args.event_level = true;
      args.summit = false;
    } else if (arg == "--summit") {
      args.summit = true;
      args.event_level = false;
    } else if ((arg == "--relay" || arg == "--connect" || arg == "--room") &&
               i + 1 < argc) {
      std::string value = argv[++i];
      if (arg == "--room") {
        if (!ghosts::valid_room(value)) {
          std::cerr << "Room: 1..23 letters, digits, - or _\n";
          std::exit(2);
        }
        args.room = value;
      } else {
        std::string port = value;
        if (arg == "--connect") {
          auto split = value.rfind(':');
          args.ghost_host =
              split == std::string::npos ? value : value.substr(0, split);
          port = split == std::string::npos ? "27888" : value.substr(split + 1);
          if (args.ghost_host.empty()) {
            std::cerr << "Missing relay host\n";
            std::exit(2);
          }
        }
        if (port.empty() ||
            port.find_first_not_of("0123456789") != std::string::npos ||
            port.size() > 5 || std::stoul(port) == 0 ||
            std::stoul(port) > 65535) {
          std::cerr << "UDP port must be 1..65535\n";
          std::exit(2);
        }
        if (arg == "--relay")
          args.relay_port = std::stoi(port);
        else
          args.ghost_port = port;
      }
    } else if (arg == "--seed" && i + 1 < argc) {
      std::string value = argv[++i];
      size_t used = 0;
      try {
        auto v = std::stoull(value, &used);
        if (used != value.size() || v > UINT32_MAX || value[0] == '-')
          throw std::invalid_argument("seed");
        args.seed = uint32_t(v);
      } catch (...) {
        std::cerr << "Seed must be 0..4294967295\n";
        std::exit(2);
      }
    } else if (arg == "--ssh")
      args.input_mode = InputMode::Ssh;
    else if (arg == "--ssh-basic")
      args.input_mode = InputMode::SshBasic;
    else if (arg == "--demo") {
      args.demo = true;
    } else if (arg == "--preview" && i + 1 < argc) {
      args.preview = argv[++i];
    } else if (arg == "--smoke") {
      args.smoke = true;
    } else if (arg == "--input-test") {
      args.input_test = true;
    } else if (arg == "--smoke-steps" && i + 1 < argc) {
      args.smoke_steps = std::max(1, std::atoi(argv[++i]));
    } else if (arg == "--cell-aspect" && i + 1 < argc) {
      args.cell_aspect = std::atof(argv[++i]);
    } else if (arg == "--input" && i + 1 < argc) {
      const std::string mode = argv[++i];
      if (mode == "evdev") {
        args.input_mode = InputMode::Evdev;
      } else if (mode == "terminal") {
        args.input_mode = InputMode::Terminal;
      } else if (mode == "auto") {
        args.input_mode = InputMode::Auto;
      } else {
        std::cerr << "unknown --input mode: " << mode << "\n";
        std::exit(2);
      }
    } else if (arg == "-h" || arg == "--help") {
      std::cout
          << "usage: terminal_velocity [--smoke] [--input-test] [--smoke-steps "
             "N] [--cell-aspect R] [--input evdev|terminal|auto] [--event] "
             "[--seed N] [--summit] [--ssh | --ssh-basic]\n"
             "  --relay PORT          Run a headless UDP ghost relay\n"
             "  --connect HOST[:PORT] Join live ghosts (default port 27888)\n"
             "  --room NAME           Match friends in the same course/seed\n";
      std::exit(0);
    } else if (debug_frontend &&
               (arg == "--screenshot" || arg == "--scene" ||
                arg == "--frames") &&
               i + 1 < argc) {
      ++i;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      std::exit(2);
    }
  }
  return args;
}

} // namespace

#ifndef TERMINAL_VELOCITY_NO_TUI_MAIN
int main(int argc, char **argv) {
  const Args args = parse_args(argc, argv);
  if (args.relay_port) {
    try {
      ghosts::Relay relay(uint16_t(args.relay_port));
      std::signal(SIGINT, [](int) { stop_requested = true; });
      std::signal(SIGTERM, [](int) { stop_requested = true; });
      std::cout << "Ghost relay listening on UDP " << relay.port() << std::endl;
      while (!stop_requested) {
        relay.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      return 0;
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      return 1;
    }
  }
  const double aspect =
      clamp(args.cell_aspect > 0.0 ? args.cell_aspect : detect_cell_aspect(),
            1.1, 3.4);
  if (!args.preview.empty()) {
    auto g = args.summit        ? build_summit()
             : args.event_level ? build_level()
                                : build_procedural(args.seed);
    auto s = new_state(g);
    s.menu = args.preview == "menu";
    s.journal = args.preview == "journal";
    s.won = args.preview == "win";
    s.input_source = "evdev";
    update_camera(s, g, 110, 27, aspect, 10);
    RenderGrid(g, s, 34, 110, aspect).draw();
    return 0;
  }
  if (args.input_test) {
#ifdef _WIN32
    DWORD mode = 0;
    bool console = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
    std::cout << (console ? "Windows console input ready\n"
                          : "Run in Windows Terminal\n");
    return console ? 0 : 1;
#else
    EvdevInput evdev;
    if (!evdev.active()) {
      std::cerr
          << "evdev unavailable: no readable keyboard event devices found\n";
      for (const std::string &line : evdev.diagnostics()) {
        std::cerr << "  " << line << "\n";
      }
      std::cerr << "Run ./play.sh to open keyboard descriptors for this "
                   "session only.\n"
                << "The helper drops root before starting the game.\n";
      return 1;
    }
    std::cout << "evdev ok: " << evdev.device_count()
              << " keyboard event device(s)\n";
    for (const std::string &line : evdev.diagnostics()) {
      std::cout << "  " << line << "\n";
    }
    return 0;
#endif
  }
  if (args.smoke) {
    return run_smoke(aspect, args.smoke_steps);
  }
  try {
    return run_game(aspect, args.input_mode, args.demo, args.event_level,
                    args.seed, args.summit, args.ghost_host, args.ghost_port,
                    args.room);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
#endif
