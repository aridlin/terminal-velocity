#include "audio.hpp"
#include "ghost_net.hpp"
#include "progress.hpp"
#include "iceberg_types.hpp"
#include "iceberg_save.hpp"
#include "iceberg_story.hpp"
#include "iceberg_epilogue_story.hpp"
#include "iceberg_inspection.hpp"
#include "iceberg_ui.hpp"
#include <csignal>
#ifndef _WIN32
#include <fcntl.h>
#include <fstream>
#ifndef __EMSCRIPTEN__
#include <linux/input.h>
#endif
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
#include <functional>

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
  double momentum_left = 0, dash_jump_left = 0, wall_bounce_left = 0;
  double lift_x = 0, lift_y = 0, lift_left = 0;

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
  bool next_seed = false, menu_request = false, mute_audio = false;
  bool interact = false, inspect = false, map = false, language = false;
  int navigate = 0;
  std::optional<int> move_pressed={}, aim_pressed={};
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
  iceberg::View adventure;
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
  std::vector<double> platform_armed_at;
  std::vector<ghosts::Ghost> ghosts;
  bool multiplayer = false;
  std::array<ChapterRecord, 3> records{};
  int unlocked = 1, earned_stars = 0;
  bool result_recorded = false, new_best = false, progress_error = false;
  std::string audio_error;
  bool audio_muted = false;
  struct Mote {
    double x, y, vx, vy, life;
    bool gold;
  };
  std::vector<Mote> motes;
  struct Afterimage {
    double x, y, life;
  };
  std::vector<Afterimage> afterimages;
  double step_distance = 0, trail_clock = 0, pickup_flash = 0;
  int step_variant = 0;
  std::string moment;
  double moment_left = 0;
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
  bool tram = false;
  bool fragile = false;
  bool icy = false;
  static constexpr double CrackTime = .55, RestoreTime = 2.9;
  double crack_age(const GameState &state, size_t index, double time) const {
    const double start = state.platform_armed_at[index];
    return start > 0 ? time - start : RestoreTime;
  }
  bool available(const GameState &state, size_t index, double time) const {
    if (!fragile) return true;
    const double age = crack_age(state, index, time);
    return age < CrackTime || age >= RestoreTime;
  }
  double offset(double t) const {
    const double cycle=t/period+phase;
    if(!tram)return std::sin(cycle*6.283185307179586);
    const double half_cycle=std::fmod(cycle,1.)*2;
    const double travel=std::clamp((std::fmod(half_cycle,1.)-.22)/.78,0.,1.);
    const double eased=travel*travel*(3-2*travel);
    return half_cycle<1?2*eased-1:1-2*eased;
  }
  Rect at(double t) const {
    const double a=offset(t);
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
enum class InteractionKind { Npc, Board, Valve, Note, Task, Secret, Return, Shortcut, Replay, Rescue, Carry, Deliver, Evidence, Epilogue, RouteChoice, DockWorker };
struct Interaction {
  Rect area;
  InteractionKind kind;
  int id;
  iceberg::Text label;
  int detail = 0;
};
struct SortingBelt { double x, y, width, speed; };
struct SpringPad : Rect {
  double push_x, push_y;
  SpringPad(double x,double y,double w,double h,double push_x=0.,double push_y=-31.)
    :Rect{x,y,w,h},push_x(push_x),push_y(push_y){}
};
struct AirField {
  Rect area;
  double ax=0, ay=0, period=0, phase=0;
  uint64_t disabled_by=0;
  bool reverses=false;
  bool active(uint64_t flags) const {return !disabled_by||(flags&disabled_by)!=disabled_by;}
  double strength(double elapsed) const {
    if(period<=0)return 1.;
    const double wave=std::sin((elapsed/period+phase)*6.283185307);
    return reverses?wave:.5+.5*wave;
  }
};
struct Gate { Rect area; uint64_t flag = 0; };
struct GrowingBridge { Rect area; uint64_t flag = 0; double amount = 1.; };
struct PulseHazard {
  Rect area;
  double period=4,phase=0;
  double cycle(double t) const {return std::fmod(t/period+phase,1.);}
  bool active(double t) const {return cycle(t)<.24;}
  static constexpr double warning_start=.82;
  bool warning(double t) const {return cycle(t)>warning_start;}
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
    for (const auto &gate : gates)
      if ((switch_flags & gate.flag) != gate.flag && gate.area.intersects({double(col),double(row),1,1}))
        return true;
    for (const auto &bridge : growth) {
      const double width=std::floor(bridge.area.w*bridge.amount);
      if(width>0 && (switch_flags&bridge.flag)==bridge.flag &&
         Rect{bridge.area.x,bridge.area.y,width,bridge.area.h}.intersects({double(col),double(row),1,1}))return true;
    }
    return solid_[index(col, row)];
  }

  void protect_exit() {
    if (!adventure_level || goal.x < 0) return;
    const int floor = int(std::lround(goal.bottom()));
    const int edge = int(std::ceil(goal.right()));
    const int wall = edge + 1;
    if (floor < 5 || floor >= height_ || wall >= width_ || width_ - edge > 16 ||
        !solid_at(int(std::floor(goal.x + goal.w * .5)), floor)) return;
    bool drop = false;
    for (int x = edge; x < std::min(width_, edge + 8); ++x)
      drop = drop || !solid_at(x, floor);
    if (!drop) return;
    for (int y = floor - 5; y <= floor; ++y)
      if (hazard_at(wall, y) || hazard_at(edge, y)) return;
    add_solid_rect(edge, floor, 2, 1);
    add_solid_rect(wall, floor - 5, 1, 5);
    exit_wall_x_ = wall;
  }

  bool touches_exit(const Rect &rect) const {
    if (!event_level && !adventure_level) return rect.right() >= goal.x;
    Rect entrance = goal;
    if (exit_wall_x_ >= 0) entrance.w = exit_wall_x_ - entrance.x;
    return rect.intersects(entrance);
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
  bool adventure_level = false;
  int verification_policy = 0, generation_fallback = 0;
  int district = 0, template_id = -1;
  iceberg::RoomRole room_role = iceberg::RoomRole::Traverse;
  iceberg::Text room_name;
  std::vector<std::pair<double,double>> checkpoint_points, route_points;
  std::vector<Interaction> interactions;
  std::vector<AirField> air_fields;
  std::vector<SortingBelt> sorting_belts;
  std::vector<Rect> ice_strips;
  bool on_ice(const Player &player) const {
    if(!player.grounded)return false;
    if(player.support>=0&&size_t(player.support)<platforms.size()&&platforms[size_t(player.support)].icy)return true;
    const double center=player.x+kPlayerWidth*.5,feet=player.rect().bottom();
    return std::any_of(ice_strips.begin(),ice_strips.end(),[&](const Rect &strip){
      return center>=strip.left()&&center<strip.right()&&std::abs(feet-strip.top())<.12;
    });
  }
  std::vector<Rect> water_basins; // Scenery behind the walkable surfaces.
  std::vector<PulseHazard> pulses;
  std::vector<Gate> gates;
  std::vector<GrowingBridge> growth;
  uint64_t switch_flags = 0, goal_requires = 0, dash_lock = 0, dash_unlock = 0;
  bool dash_locked() const {return (switch_flags&dash_lock)&&!(switch_flags&dash_unlock);}
  bool summit = false;
  int chapter = 0;
  uint32_t seed = 0;
  std::vector<std::pair<double, double>> sweets;
  std::vector<double> checkpoints;
  std::vector<MovingPlatform> platforms;
  std::vector<Patrol> enemies;
  std::vector<SpringPad> springs;
  std::vector<std::pair<double, double>> refills;
  double spawn_x = 5.0;
  double spawn_y = 36.0 - kPlayerHeight;
  Rect goal{173.0, 13.0, 3.0, 5.0};

private:
  int exit_wall_x_ = -1;
  bool inside(int col, int row) const {
    return col >= 0 && col < width_ && row >= 0 && row < height_;
  }

  int index(int col, int row) const { return row * width_ + col; }

  int width_ = 0;
  int height_ = 0;
  std::vector<bool> solid_;
  std::vector<bool> hazard_;
};

#include "iceberg_world.hpp"
#include "iceberg_secrets.hpp"

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

const char *chapter_name(int chapter) {
  static const char *names[] = {"", "ZACISZE", "PRZEŁĘCZ", "OSTATNIA GRAŃ"};
  return names[std::clamp(chapter, 0, 3)];
}

EngineGrid build_chapter(int chapter) {
  chapter = std::clamp(chapter, 1, 3);
  const int rooms = chapter == 3 ? 5 : 4;
  const int span = chapter == 1 ? 30 : chapter == 2 ? 32 : 34;
  const int floor_width = chapter == 1 ? 23 : chapter == 2 ? 21 : 22;
  EngineGrid g(8 + rooms * span + 16, 46);
  g.event_level = false;
  g.chapter = chapter;
  g.spawn_x = 4;
  g.spawn_y = 38 - kPlayerHeight;
  g.goal = {double(g.width() - 8), 33, 3, 5};
  g.checkpoints = {4};
  g.add_solid_rect(0, 38, 8, 8);
  for (int room = 0; room < rooms; ++room) {
    int x = 8 + room * span;
    g.add_solid_rect(x, 38, floor_width, 8);
    g.checkpoints.push_back(x + 2);
    int height = chapter == 1 ? 1 + room % 2 : 2 + room % 3;
    g.add_solid_rect(x + 7, 38 - height, 3, height);
    g.sweets.push_back({double(x + 8), double(35 - height)});
    g.sweets.push_back({double(x + floor_width - 1), 33});
    g.refills.push_back({x + floor_width + (span - floor_width) * .5, 34.5});
    if (chapter == 1) {
      g.add_solid_rect(x + 13, 32, 4, 1);
      g.sweets.push_back({double(x + 14), 29});
    } else if (chapter == 2) {
      g.platforms.push_back(
          {{double(x + 15), 31, 3, .5}, 2, 1, 4.2, room * .2});
      g.add_solid_rect(x + 2, 30, 4, 1);
      g.springs.push_back({double(x + 3), 29.7, 1.3, .3});
      g.sweets.push_back({double(x + 16), 27.5});
      if (room >= 2)
        g.add_hazard_rect(x + 17, 37, 2);
    } else {
      g.add_solid_rect(x + 13, 29, 2, 5);
      g.add_solid_rect(x + 17, 26, 4, 1);
      g.sweets.push_back({double(x + 18), 23});
      g.add_hazard_rect(x + 16, 37, 2);
    }
  }
  g.add_solid_rect(8 + rooms * span, 38, 16, 8);
  return g;
}

int course_id(const EngineGrid &g) {
  return g.chapter ? 2 + g.chapter : g.summit ? 2 : g.event_level ? 0 : 1;
}

GameState new_state(const EngineGrid &grid) {
  GameState state;
  state.collected.resize(grid.sweets.size());
  state.defeated.resize(grid.enemies.size());
  state.refill_cooldown.resize(grid.refills.size());
  if (std::any_of(grid.platforms.begin(), grid.platforms.end(),
                  [](const auto &platform) { return platform.fragile; }))
    state.platform_armed_at.resize(grid.platforms.size());
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
  std::fill(state.platform_armed_at.begin(), state.platform_armed_at.end(), 0);
  state.player.support = -1;
  state.player.drop_left = 0;
  state.player.momentum_left = state.player.dash_jump_left = state.player.wall_bounce_left = 0;
  state.player.lift_x = state.player.lift_y = state.player.lift_left = 0;
  state.won = false;
  state.result_recorded = false;
  state.earned_stars = 0;
  state.new_best = false;
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
    ++state.deaths;
    reset_player(state);
    return;
  }

  player.wall_lock_left = std::max(0.0, player.wall_lock_left - dt);
  player.jump_buffer_left = std::max(0.0, player.jump_buffer_left - dt);
  player.dash_buffer_left = std::max(0.0, player.dash_buffer_left - dt);
  player.coyote_left = std::max(0.0, player.coyote_left - dt);
  player.momentum_left = std::max(0., player.momentum_left - dt);
  player.dash_jump_left = std::max(0., player.dash_jump_left - dt);
  player.wall_bounce_left = std::max(0., player.wall_bounce_left - dt);
  player.lift_left = std::max(0., player.lift_left - dt);
  if (controls.jump_pressed) {
    player.jump_buffer_left = kJumpBuffer;
  }
  if (controls.dash_pressed && !grid.dash_locked()) {
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
    // These techniques apply to adventure physics, leaving legacy time records
    // comparable. The input window is forgiving; the route decides difficulty.
    if (grid.adventure_level && player.jump_buffer_left > 0) {
      bool floor = player.grounded;
      auto probe = player.rect(); probe.y += .14;
      for (auto r : grid.solid_rects_near(probe)) floor |= probe.intersects(r);
      int wall = wall_dir(grid, player);
      if (floor && std::abs(player.vx) > 12 && player.vy >= -2) {
        player.vx = std::copysign(26., player.vx);
        player.vy = -kJumpSpeed;
        player.dash_time_left = 0;
        player.jump_buffer_left = 0;
        player.grounded = false;
        player.momentum_left = .42;
        player.dash_jump_left = .8;
      } else if (wall && player.vy < -10) {
        player.vx = -wall * 20.; player.vy = -29.;
        player.dash_time_left = 0; player.jump_buffer_left = 0;
        player.wall_lock_left = .17; player.momentum_left = .35;
        player.wall_bounce_left = .8; player.facing = -wall;
      }
    }
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
    if ((grid.switch_flags&grid.goal_requires)==grid.goal_requires &&
        grid.touches_exit(rect)) {
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
      if (grid.adventure_level && player.lift_left > 0) {
        player.vx += clamp(player.lift_x, -12., 12.);
        player.vy += clamp(player.lift_y, -12., 0.);
        player.lift_left = 0;
        player.momentum_left = .35;
      }
    } else if (player.wall_coyote_left > 0) {
      player.vx = -player.last_wall * kWallJumpX;
      player.vy = -kWallJumpY;
      player.facing = -player.last_wall;
      player.wall_coyote_left = 0;
      player.wall_lock_left = 0.13;
      player.jump_buffer_left = 0.0;
    }
  }

  const bool ice=grid.on_ice(player);
  if (controls.move != 0 && player.wall_lock_left <= 0.0) {
    player.facing = controls.move;
    const double apex_bonus =
        (std::abs(player.vy) < 3.0 && !player.grounded) ? kApexAccelBonus : 0.0;
    const double accel = player.grounded ? (ice ? 46.0 : kMoveAccel) : kAirAccel + apex_bonus;
    const double target = controls.move * kMaxRunSpeed;
    if (!(player.momentum_left > 0 && !player.grounded &&
          player.vx * controls.move > kMaxRunSpeed))
      player.vx += clamp(target - player.vx, -accel * dt, accel * dt);
  } else if (player.grounded) {
    player.vx = approach_zero(player.vx, (ice ? 20.0 : kGroundFriction) * dt);
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
  if ((grid.switch_flags&grid.goal_requires)==grid.goal_requires &&
      grid.touches_exit(rect)) {
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
    const auto &platform = g.platforms[p.support];
    auto old = platform.at(before_time), now = platform.at(before_time + dt);
    if (!platform.available(s, size_t(p.support), before_time + dt)) {
      p.grounded = false;
      p.support = -1;
    } else if (c.fast_fall) {
      p.drop_left = .22;
      p.grounded = false;
      p.support = -1;
    } else {
      if (g.adventure_level) {
        p.lift_x = (now.x - old.x) / dt;
        p.lift_y = (now.y - old.y) / dt;
        p.lift_left = .12;
      }
      p.x += now.x - old.x;
      p.y += now.y - old.y;
    }
  }
  if (p.grounded && p.support < 0) {
    for (const auto &belt : g.sorting_belts) {
      const auto feet = p.rect();
      if (std::abs(feet.bottom() - belt.y) > .1 ||
          feet.right() <= belt.x || feet.left() >= belt.x + belt.width) continue;
      double run_speed = p.vx;
      p.vx = belt.speed;
      move_axis(g, p, dt, 'x');
      p.vx = run_speed;
      p.lift_x = belt.speed;
      p.lift_y = 0;
      p.lift_left = .12;
      break;
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
  for (const auto &field : g.air_fields)
    if (field.active(g.switch_flags) && p.rect().intersects(field.area) && p.dash_time_left <= 0) {
      double amount = field.strength(s.elapsed);
      p.vx += field.ax * amount * dt;
      p.vy += field.ay * amount * dt;
      p.vx = clamp(p.vx, -38., 38.);
      p.vy = clamp(p.vy, -38., kMaxFallSpeed);
    }
  if (p.drop_left <= 0 && p.vy >= 0) {
    for (size_t i = 0; i < g.platforms.size(); ++i) {
      const auto &platform = g.platforms[i];
      if (!platform.available(s, i, s.elapsed)) continue;
      auto r = platform.at(s.elapsed), prev = platform.at(before_time);
      if (old.bottom() <= prev.top() + .10 && p.rect().bottom() >= r.top() &&
          p.rect().right() > r.left() && p.x < r.right()) {
        p.y = r.y - kPlayerHeight;
        p.vy = 0;
        p.grounded = true;
        p.dash_available = true;
        p.coyote_left = kCoyoteTime;
        p.support = int(i);
        if (platform.fragile && platform.crack_age(s, i, s.elapsed) >= MovingPlatform::RestoreTime)
          s.platform_armed_at[i] = s.elapsed;
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
      p.vy = spring.push_y;
      if (spring.push_x != 0) {
        p.vx = spring.push_x;
        p.facing = spring.push_x > 0 ? 1 : -1;
        p.dash_time_left = 0;
        p.momentum_left = .5;
      }
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
  for(const auto &pulse:g.pulses)if(pulse.active(s.elapsed)&&p.rect().intersects(pulse.area)) {
    ++s.deaths;reset_player(s);return;
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
    if(input.move_pressed)pending.move_pressed=input.move_pressed;
    if(input.aim_pressed)pending.aim_pressed=input.aim_pressed;
    const int held_move=input.move, held_aim=input.aim_y;
    accumulator += clamp(elapsed, 0.0, 0.1);
    while (accumulator + 1e-10 >= 1.0 / 120.0) {
      // A released direction can still aim a queued dash, then expires.
      input.move=held_move?held_move:pending.move_pressed.value_or(0);
      input.aim_y=held_aim?held_aim:pending.aim_pressed.value_or(0);
      input.jump_pressed = pending.jump_pressed;
      input.dash_pressed = pending.dash_pressed;
      input.restart = pending.restart;
      pending = Controls{};
      // Adventure goals indicate current doorway contact, while classic
      // courses latch completion for the results screen.
      if (grid.adventure_level) state.won = false;
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
        if (grid.adventure_level) {
          for (size_t cp = 1; cp < grid.checkpoint_points.size(); ++cp) {
            auto [x,y] = grid.checkpoint_points[cp];
            if (int(cp) > state.checkpoint && std::abs(state.player.x - x) < 3 &&
                std::abs(state.player.y - y) < .3) {
              state.checkpoint = int(cp); state.notice = 1.5;
              state.spawn_x = x; state.spawn_y = y;
            }
          }
        }
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

void attach_progress(GameState &s, const ProgressStore &progress) {
  s.records = progress.records;
  s.unlocked = progress.unlocked;
}
void finish_chapter(GameState &s, const EngineGrid &g,
                    ProgressStore &progress) {
  if (!g.chapter || !s.won || s.result_recorded)
    return;
  s.result_recorded = true;
  s.earned_stars = run_stars(s.deaths, s.sweets_count(), int(g.sweets.size()));
  s.progress_error =
      !progress.record(g.chapter, s.elapsed, s.deaths, s.sweets_count(),
                       int(g.sweets.size()), s.new_best);
  attach_progress(s, progress);
  s.earned_stars = progress.records[size_t(g.chapter - 1)].stars;
}
void burst(GameState &s, double x, double y, int count, bool gold) {
  for (int i = 0; i < count && s.motes.size() < 96; ++i) {
    double angle = i * 6.2831853 / count;
    s.motes.push_back({x, y, std::cos(angle) * 3.5, std::sin(angle) * 3.5 - 1,
                       .30 + (i % 3) * .07, gold});
  }
}
void feedback_step(GameState &s, const EngineGrid &g, const Player &before,
                   int old_deaths, int old_pickups, int old_checkpoint,
                   bool was_won, double dt, GameAudio &audio) {
  dt = clamp(dt, 0., .1);
  for (auto &m : s.motes) {
    m.life -= dt;
    m.x += m.vx * dt;
    m.y += m.vy * dt;
    m.vy += dt * 6;
  }
  std::erase_if(s.motes, [](const auto &m) { return m.life <= 0; });
  for (auto &t : s.afterimages)
    t.life -= dt;
  std::erase_if(s.afterimages, [](const auto &t) { return t.life <= 0; });
  s.moment_left = std::max(0., s.moment_left - dt);
  s.pickup_flash = std::max(0., s.pickup_flash - dt);
  auto &p = s.player;
  if (s.deaths != old_deaths) {
    audio.play(AudioCue::Death);
    burst(s, before.x, before.y + 1, 12, false);
    s.step_distance = 0;
    s.afterimages.clear();
    return;
  }
  if (p.dash_time_left > before.dash_time_left) {
    audio.play(AudioCue::Dash);
    burst(s, p.x, p.y + .8, 6, true);
  } else if (before.vy >= -1 && p.vy < -8 && !p.grounded) {
    audio.play(p.vy < -27 ? AudioCue::Spring : AudioCue::Jump);
  }
  if (!before.grounded && p.grounded && before.vy > 4) {
    audio.play(AudioCue::Land);
    burst(s, p.x + .45, p.y + kPlayerHeight, 5, false);
  }
  if (!before.dash_available && p.dash_available && !p.grounded) {
    audio.play(AudioCue::Refill);
    s.moment = "ZRYW ODNOWIONY";
    s.moment_left = .7;
    burst(s, p.x + .45, p.y + .8, 8, true);
  }
  if (s.sweets_count() > old_pickups) {
    audio.play(AudioCue::Pickup);
    s.pickup_flash = .25;
    s.moment = "+" + std::to_string(s.sweets_count() - old_pickups);
    s.moment_left = .7;
    burst(s, p.x + .45, p.y + .5, 10, true);
  }
  if (s.checkpoint != old_checkpoint)
    audio.play(AudioCue::Checkpoint);
  if (s.won && !was_won)
    audio.play(AudioCue::Win);
  if (p.grounded && before.grounded && std::abs(p.vx) > 2) {
    s.step_distance += std::abs(p.x - before.x);
    if (s.step_distance > 2.3) {
      s.step_distance = 0;
      audio.play((s.step_variant++ % 2) ? AudioCue::Step1 : AudioCue::Step2);
    }
  }
  s.trail_clock += dt;
  if (p.dash_time_left > 0 && s.trail_clock > .025) {
    s.trail_clock = 0;
    if (s.afterimages.size() < 8)
      s.afterimages.push_back({before.x, before.y, .14});
  }
  (void)g;
}

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
  const double scale=grid.adventure_level?.40:kWorldUnitsPerRenderCol;
  const double visible_w = view_width * scale;
  const double visible_h = play_height * aspect * scale;
  double target_x = state.player.x - visible_w * 0.42 + state.player.vx * 0.15;
  double target_y = state.player.y - visible_h * (grid.adventure_level?.68:.55) + state.player.vy * 0.065;
  target_x = clamp(target_x, 0.0, std::max(0.0, grid.width() - visible_w));
  target_y = clamp(target_y, 0.0, std::max(0.0, grid.height() - visible_h));
  const double blend = 1.0 - std::exp(-dt * 10.5);
  state.camera_x += (target_x - state.camera_x) * blend;
  state.camera_y += (target_y - state.camera_y) * blend;
}

#ifndef __EMSCRIPTEN__
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
    c.interact = press(KEY_E) || press(KEY_F);
    c.inspect = press(KEY_B);
    c.map = press(KEY_J);
    c.language = press(KEY_L);
    c.navigate = int(press(KEY_DOWN) || press(KEY_S)) - int(press(KEY_UP) || press(KEY_W));
    c.select_level = press(KEY_1)   ? 1
                     : press(KEY_2) ? 2
                     : press(KEY_3) ? 3
                     : press(KEY_4) ? 4
                     : press(KEY_5) ? 5
                     : press(KEY_6) ? 6
                                    : 0;
    c.mute_audio = press(KEY_U);
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
#ifdef _WIN32
#include <process.h>
#endif
#include "windows_console.hpp"
#endif

#endif
enum class Color {
  Gold,
  Ghost,
  Moss,
  Ice,
  Trail,
  ForestShade,
  Fog,
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
  uint32_t rgb = 0;
  uint32_t background = 0;
};
using CellFrame = std::vector<std::vector<Cell>>;
std::function<void(const CellFrame &)> cell_frame_sink;

uint32_t cell_color(const Cell &cell) {
  if (cell.rgb) return cell.rgb;
  switch (cell.color) {
  case Color::Gold: case Color::Goal: return 0xf4cd5e;
  case Color::Ghost:return 0x5dc9d2;
  case Color::Moss:return 0x4c776b;
  case Color::Ice:return 0x7497b1;
  case Color::Trail:return 0x96753e;
  case Color::ForestShade:return 0x273a33;
  case Color::Fog:return 0x2b3648;
  case Color::Decor:return 0x412a31;
  case Color::Solid:return 0x7e414d;
  case Color::Hazard:return 0xeb394f;
  case Color::Player:return 0xfaf7ef;
  default:return 0xc8bbb8;
  }
}
uint32_t cell_background(const Cell &cell) {return cell.background?cell.background:0x191418;}

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
  case Color::Moss:
    out += "\x1b[38;2;76;119;107m";
    break;
  case Color::Ice:
    out += "\x1b[38;2;116;151;177m";
    break;
  case Color::Trail:
    out += "\x1b[38;2;150;117;62m";
    break;
  case Color::ForestShade:
    out += "\x1b[38;2;39;58;51m";
    break;
  case Color::Fog:
    out += "\x1b[38;2;43;54;72m";
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
        cols_(std::max(1, cols)), aspect_(aspect), scale_(grid.adventure_level?.40:kWorldUnitsPerRenderCol),
        play_rows_(std::max(1, rows - 7)) {
    buffer_.assign(rows_, std::vector<Cell>(cols_));
  }
  void draw() {
    if (rows_ < (grid_.adventure_level&&cols_>=56?20:24) || cols_ < (grid_.adventure_level?34:76)) {
      text(1, 2, grid_.adventure_level?iceberg::tr(state_.adventure.language,"Powiększ okno: 34 × 24 lub 56 × 20.","Resize the window: 34 × 24 or 56 × 20."):"Powiększ terminal do co najmniej 76 × 24.");
      text(3, 2, grid_.adventure_level?iceberg::tr(state_.adventure.language,"Zalecane: 110 × 34. Q — wyjście.","Recommended: 110 × 34. Q — quit."):"Zalecane: 110 × 34. Q — wyjście.");
      flush();
      return;
    }
    if (grid_.adventure_level) {
      draw_iceberg();
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
      if (grid_.event_level) {
        text(y, x, "│██", Color::Player);
        text(y + 1, x, "│██", Color::Hazard);
      } else {
        text(y, x, "│◆",
             int(i) <= state_.checkpoint ? Color::Gold : Color::Ghost);
        text(y + 1, x, "│", Color::Decor);
      }
      text(y + 2, x, "│", Color::Gold);
    }
    for (auto platform : grid_.platforms) {
      auto r = platform.at(state_.elapsed);
      auto [x, y] = screen(r.x, r.y);
      text(y, x,
           std::string(std::max(1, int(r.w / scale_)), '='),
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
    text(gy, gx, grid_.chapter ? "SZLAK" : "META", Color::Gold);
    if (grid_.event_level) {
      auto [px, py] = screen(state_.player.x - .15, state_.player.y - .9);
      text(py, px, "JP2", Color::Gold);
    }
    for (size_t i = 0; i < grid_.refills.size(); ++i) {
      auto [x, y] = screen(grid_.refills[i].first, grid_.refills[i].second);
      text(y, x, state_.refill_cooldown[i] > 0 ? "◇" : "◆",
           state_.refill_cooldown[i] > 0 ? Color::Decor : Color::Ghost);
    }
    for (const auto &m : state_.motes)
      if (!grid_.solid_at(int(std::floor(m.x)), int(std::floor(m.y))) &&
          !state_.player.rect().intersects({m.x, m.y, .05, .05})) {
        auto [x, y] = screen(m.x, m.y);
        if (y >= 4 && y < rows_ - 3)
          text(y, x, m.life > .2 ? "·" : "˙",
               m.gold ? Color::Gold : Color::Ghost);
      }
    fill(0, 0, cols_, 3, Color::Default);
    text(0, 0, "█", Color::Player);
    text(1, 0, "█", Color::Hazard);
    text(0, 2,
         grid_.chapter       ? "WSPINACZKA / " + std::to_string(grid_.chapter) +
                                   " / " + chapter_name(grid_.chapter)
         : grid_.event_level ? "EVENT / JP2: SZTAFETA WOLNOŚCI"
         : grid_.summit      ? "TERMINAL VELOCITY / SZCZYT"
                             : "TERMINAL VELOCITY / EKSPEDYCJA",
         Color::Gold);
    if (state_.multiplayer)
      text(0, cols_ - 12, "DUCHY " + std::to_string(state_.ghosts.size()),
           Color::Ghost);
    int chapter = state_.player.x < 60 ? 0 : state_.player.x < 118 ? 1 : 2;
    const char *names[] = {"01 WADOWICE", "02 KRAKÓW", "03 WARSZAWA"};
    text(1, 2,
         grid_.chapter       ? "ETAP " + std::to_string(grid_.chapter) +
                                   "/3   •   DOTRZYJ DO KOŃCA SZLAKU"
         : grid_.event_level ? std::string(names[chapter]) +
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
    std::string bottom =
        "U dźwięk: " + std::string(state_.audio_muted ? "wył." : "wł.") +
        "   •   TAB ruch i cele";
    if (grid_.chapter && state_.records[size_t(grid_.chapter - 1)].complete)
      bottom += "   •   Rekord " +
                fixed(state_.records[size_t(grid_.chapter - 1)].best_time) +
                " s";
    if (state_.debug)
      bottom = "F3 debug • " + state_.input_source + " • 120 Hz";
    text(rows_ - 1, 2, bottom, Color::Gold);
    if (state_.moment_left > 0 && state_.notice <= 0)
      center(5, state_.moment, Color::Gold);
    if (!state_.audio_error.empty())
      text(rows_ - 1, 2, state_.audio_error, Color::Text);
    if (state_.notice > 0) {
      center(5, "PUNKT ZAPISU — R wraca tutaj", Color::Gold);
      center(6,
             !grid_.event_level
                 ? "Możesz próbować dalej. R wraca do tego punktu."
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
  static constexpr uint32_t water_surface_background=0x214b62;
#include "iceberg_render.hpp"
  const EngineGrid &grid_;
  const GameState &state_;
  int rows_, cols_;
  double aspect_,scale_;
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
    if (row >= 0 && row < rows_ && col >= 0 && col < cols_) {
      if(!c.background)c.background=buffer_[row][col].background;
      buffer_[row][col] = std::move(c);
    }
  }
  void fill(int row, int col, int w, int h, Color c) {
    for (int y = row; y < row + h; ++y)
      for (int x = col; x < col + w; ++x)
        put(y, x, {" ", c,false,false,0,0,iceberg_base_background()});
  }
  void text(int row, int col, const std::string &s, Color c = Color::Text) {
    for (auto ch : glyphs(s))
      put(row, col++, {ch, c, false, false});
  }
  void center(int row, const std::string &s, Color c = Color::Text) {
    text(row, (cols_ - int(glyphs(s).size())) / 2, s, c);
  }
  std::pair<int, int> screen(double x, double y) const {
    return {int(std::floor((x - state_.camera_x) / scale_)),
            4 + int(std::floor((y - state_.camera_y) /
                               (aspect_ * scale_)))};
  }
  Cell world_cell(int x, int y) const {
    double wx = state_.camera_x + x * scale_,
           wy = state_.camera_y + y * aspect_ * scale_;
    unsigned solid = 0, edge = 0, hazard = 0, body = 0, goal = 0, ghost = 0,
             trail = 0;
    for (int dy = 0; dy < 4; ++dy)
      for (int dx = 0; dx < 2; ++dx) {
        double a = wx + (dx + .5) * scale_ / 2,
               b = wy + (dy + .5) * aspect_ * scale_ / 4;
        int col = int(std::floor(a)), row = int(std::floor(b));
        unsigned dot = braille_dot(dx, dy);
        if (state_.player.rect().intersects({a, b, .01, .01}))
          body |= dot;
        for (const auto &t : state_.afterimages)
          if (Rect{t.x, t.y, kPlayerWidth, kPlayerHeight}.intersects(
                  {a, b, .01, .01}))
            trail |= dot;
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
    if (body && !grid_.adventure_level)
      return braille_cell(
          body, state_.player.dash_available ? Color::Gold : Color::Player,
          true);
    if (hazard)
      return braille_cell(hazard, Color::Hazard, true);
    if (trail && !solid && !ghost)
      return braille_cell(trail, Color::Trail);
    if (ghost && !solid)
      return braille_cell(ghost, Color::Ghost);
    if (goal && !grid_.adventure_level)
      return braille_cell(goal, Color::Gold);
    if (edge)
      return braille_cell(solid, Color::Player);
    if (solid) {
      if (grid_.adventure_level) {
        auto c=solid==255?Cell{"█",Color::Solid}:braille_cell(solid,Color::Solid);
        const auto &district=iceberg::Districts[size_t(grid_.district)];
        c.rgb=grid_.district==3?0x78929e:district.stone;
        // Sparse masonry/plate seams retain a solid collision silhouette.
        if(solid==255 && (int(std::floor(wx*2))+int(std::floor(wy))*3)%47==0) {
          c.glyph=grid_.district==1?"▥":"▓";
        }
        return c;
      }
      Color terrain = grid_.chapter == 1   ? Color::Moss
                      : grid_.chapter == 2 ? Color::Ice
                                           : Color::Solid;
      return solid == 255 ? Cell{"█", terrain} : braille_cell(solid, terrain);
    }
    if (grid_.adventure_level) {
      for (const auto &water : grid_.water_basins) {
        const double depth=wy+aspect_*scale_*.5-water.y;
        if(wx<water.x||wx>=water.right()||depth<0||depth>=water.h)continue;
        const int ripple=int(wx/scale_+state_.elapsed*6);
        if(depth<aspect_*scale_)
          return {ripple%8<3?"∿":"─",Color::Decor,false,false,0,0x8ed9e5,water_surface_background};
        if(ripple%13==int(depth*2)%13)
          return {"·",Color::Decor,false,false,0,0x5196ab,0x14394e};
        return {" ",Color::Decor,false,false,0,0,0x14394e};
      }
      return iceberg_background(wx,wy,x,y);
    }
    if (grid_.chapter) {
      // Two distant ridgelines; decoration never enters collision geometry.
      double near_ridge =
          25 +
          std::abs(std::fmod(wx * .35 + grid_.chapter * 7, 22.) - 11) * .65;
      double far_ridge = 19 + std::abs(std::fmod(wx * .15 + 9, 30.) - 15) * .55;
      Color shade = grid_.chapter == 1 ? Color::ForestShade : Color::Fog;
      if (wy > near_ridge)
        return {"░", shade};
      if (wy > far_ridge && (x + y) % 3 == 0)
        return {"·", shade};
      if ((x * 47 + y * 71) % 173 == 0)
        return {"·", Color::Text};
      return {};
    }
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
  std::string stars(int count) const {
    std::string result;
    for (int i = 0; i < 3; ++i)
      result += i < count ? "★" : "☆";
    return result;
  }
  void draw_menu() {
    panel();
    center(5, "T E R M I N A L   V E L O C I T Y", Color::Gold);
    center(7, "Noc zapada w dolinie. Znajdź drogę ponad chmurami.",
           Color::Player);
    for (int i = 1; i <= 3; ++i) {
      bool selected = grid_.chapter == i;
      center(8 + i,
             "[" + std::to_string(i + 3) + "] " + chapter_name(i) + "   " +
                 stars(state_.records[size_t(i - 1)].stars) +
                 (selected ? "  ◀" : ""),
             i > state_.unlocked ? Color::Decor
             : selected          ? Color::Gold
                                 : Color::Player);
    }
    center(13, "[ENTER] GRAJ", Color::Gold);
    center(15, "[1] EVENT JP2   [2] EKSPEDYCJA   [3] SZCZYT", Color::Player);
    center(17, "SPACJA skok • SHIFT/X + kierunek zryw • żółty = gotowy zryw");
    center(19, "U dźwięk   TAB ruch i cele   Q wyjście");
    if (state_.moment_left > 0)
      center(12, state_.moment, Color::Gold);
    if (rows_ >= 29) {
      center(22, "★ dotrzyj do mety   ★ bez upadków   ★ wszystkie znajdźki",
             Color::Gold);
      center(24, "Muzyka: Kevin MacLeod / incompetech.com • CC BY 4.0",
             Color::Text);
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
      center(16,
             grid_.chapter
                 ? "★ meta • ★ bez upadków • ★ komplet znajdziek. Postęp się "
                   "zapisuje."
                 : "Seed tworzy tę samą mapę. N powtarza, G losuje następną.");
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
    center(16, "U dźwięk • gra pauzuje po utracie fokusu.");
  }
  void draw_win() {
    panel();
    if (grid_.chapter) {
      center(6, grid_.chapter == 3 ? "PONAD CHMURAMI" : "SZLAK UKOŃCZONY",
             Color::Gold);
      center(8, chapter_name(grid_.chapter), Color::Player);
      center(10,
             stars(state_.earned_stars
                       ? state_.earned_stars
                       : run_stars(state_.deaths, state_.sweets_count(),
                                   int(grid_.sweets.size()))),
             Color::Gold);
      center(12, "Czas " + fixed(state_.elapsed) + " s   •   Upadki " +
                     std::to_string(state_.deaths) + "   •   Znajdźki " +
                     std::to_string(state_.sweets_count()) + "/" +
                     std::to_string(grid_.sweets.size()));
      center(14,
             state_.new_best ? "NOWY REKORD!"
                             : "★ meta • ★ bez upadków • ★ komplet znajdziek",
             Color::Gold);
      center(16,
             grid_.chapter < 3
                 ? "[ENTER] DALEJ: " +
                       std::string(chapter_name(grid_.chapter + 1))
                 : "[ENTER] WRÓĆ DO DOLINY",
             Color::Player);
      center(18, "[N] PONÓW   [M] MENU   [Q] WYJŚCIE");
      if (state_.progress_error)
        center(20, "Nie można zapisać postępu. Sprawdź uprawnienia do "
                   "katalogu profilu.");
      return;
    }
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
  void flush() {
    for(auto &row:buffer_)for(auto &cell:row)if(!cell.background)cell.background=iceberg_base_background();
    if (cell_frame_sink) {cell_frame_sink(buffer_);return;}
    static std::vector<std::vector<Cell>> previous;
    bool full = previous.size() != buffer_.size() || previous.empty() ||
                previous[0].size() != buffer_[0].size();
    std::string out = "\x1b[?2026h";
    out.reserve(rows_ * cols_ * 4);
    if (full)
      out += "\x1b[2J";
    Color current = Color::Default;
    bool bold = false;
    uint32_t last_rgb = 0;
    uint32_t last_background = 0;
    out += ansi_for(current, false, false);
    for (int y = 0; y < rows_; ++y) {
      bool positioned = false;
      for (int x = 0; x < cols_; ++x) {
        const auto &c = buffer_[y][x];
        bool changed = full || c.glyph != previous[y][x].glyph ||
                       c.color != previous[y][x].color ||
                       c.rgb != previous[y][x].rgb ||
                       c.background != previous[y][x].background ||
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
        uint32_t rgb=cell_color(c);
        uint32_t background=cell_background(c);
        if (current != c.color || bold != c.bold || rgb != last_rgb || background != last_background) {
          out += "\x1b[0m\x1b[48;2;"+std::to_string((background>>16)&255)+";"+
            std::to_string((background>>8)&255)+";"+std::to_string(background&255)+"m";
          if(c.bold)out+="\x1b[1m";
          out += "\x1b[38;2;"+std::to_string((rgb>>16)&255)+";"+
            std::to_string((rgb>>8)&255)+";"+std::to_string(rgb&255)+"m";
          current = c.color;
          bold = c.bold;
          last_rgb=rgb;
          last_background=background;
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

std::filesystem::path audio_directory() {
#ifdef _WIN32
  std::vector<wchar_t> executable(32768);
  DWORD n =
      GetModuleFileNameW(nullptr, executable.data(), DWORD(executable.size()));
  if (n && n < executable.size())
    return std::filesystem::path(executable.data()).parent_path() /
           "assets/audio";
#else
  std::error_code ec;
  auto executable = std::filesystem::canonical("/proc/self/exe", ec);
  if (!ec)
    return executable.parent_path() / "assets/audio";
#endif
  return std::filesystem::current_path() / "assets/audio";
}

#include "iceberg_validation.hpp"
#include "iceberg_runtime.hpp"

#ifndef __EMSCRIPTEN__
[[maybe_unused]] int run_game(double aspect, InputMode input_mode,
                              bool demo = false, bool event_level = false,
                              uint32_t seed = 1, bool summit = false,
                              const std::string &ghost_host = "",
                              const std::string &ghost_port = "27888",
                              const std::string &room = "default",
                              int campaign_chapter = 1, bool mute = false,
                              bool remote_audio = false) {
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
  EngineGrid grid = campaign_chapter ? build_chapter(campaign_chapter)
                    : summit         ? build_summit()
                    : event_level    ? build_level()
                                     : build_procedural(seed);
  ProgressStore progress;
  GameAudio audio;
  audio.init(audio_directory(), mute || (remote && !remote_audio));
  GameState state = new_state(grid);
  attach_progress(state, progress);
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
                   grid.hazard_at(x, 37) || !grid.solid_at(x, foot);
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
    if (c.mute_audio)
      audio.muted = !audio.muted;
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
    if (c.select_level >= 4 && c.select_level - 3 > progress.unlocked) {
      c.select_level = 0;
      state.moment = "Ukończ poprzedni etap.";
      state.moment_left = 2;
    }
    if ((c.select_level && (state.menu || state.paused || state.won)) ||
        c.next_seed) {
      if (c.next_seed) {
        event_level = false;
        summit = false;
        campaign_chapter = 0;
        ++seed;
      } else {
        event_level = c.select_level == 1;
        summit = c.select_level == 3;
        campaign_chapter = c.select_level >= 4 ? c.select_level - 3 : 0;
      }
      ++network_epoch;
      grid = campaign_chapter ? build_chapter(campaign_chapter)
             : summit         ? build_summit()
             : event_level    ? build_level()
                              : build_procedural(seed);
      state = new_state(grid);
      attach_progress(state, progress);
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
      bool campaign_end = c.confirm && !c.new_run && grid.chapter == 3;
      if (c.confirm && !c.new_run && grid.chapter) {
        campaign_chapter = grid.chapter == 3 ? 1 : grid.chapter + 1;
        grid = build_chapter(campaign_chapter);
      }
      auto label = state.input_source;
      state = new_state(grid);
      attach_progress(state, progress);
      state.ssh = remote;
      state.ssh_basic = input.basic_ssh();
      state.input_source = label;
      state.menu = campaign_end;
      c.restart = false;
      if (campaign_end)
        c.confirm = false;
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
      audio.play(AudioCue::Menu);
      c.jump_pressed = false;
      input.clear();
      stepper = FixedStepper{};
    }
    size = term_size();
    bool too_small = size.cols < 76 || size.rows < 24;
    auto scene = state.menu ? AudioScene::Menu
                 : state.paused || state.journal || too_small
                     ? AudioScene::Paused
                 : state.won ? AudioScene::Win
                             : AudioScene::Play;
    audio.update(scene, grid.chapter >= 2 || grid.summit ? 1 : 0, dt,
                 demo || input.focused);
    state.audio_error = audio.error();
    state.audio_muted = audio.muted;
    if (!state.menu && !state.paused && !state.journal && !too_small) {
      Player before = state.player;
      int deaths = state.deaths, pickups = state.sweets_count(),
          checkpoint = state.checkpoint;
      bool was_won = state.won;
      stepper.advance(state, grid, c, dt);
      feedback_step(state, grid, before, deaths, pickups, checkpoint, was_won,
                    dt, audio);
      if (!demo)
        finish_chapter(state, grid, progress);
    } else
      stepper = FixedStepper{};
    update_camera(state, grid, size.cols, std::max(1, size.rows - 7), aspect,
                  dt);
    if (c.restart || state.deaths != previous_deaths)
      ++network_epoch;
    previous_deaths = state.deaths;
    state.multiplayer = network.enabled();
    ghosts::Snapshot snapshot;
    snapshot.room = room;
    snapshot.level = course_id(grid);
    snapshot.seed =
        grid.event_level || grid.summit || grid.chapter ? 0 : grid.seed;
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
  bool adventure = true, explicit_adventure = false, terminal_view = false, district_override = false;
  iceberg::Mode adventure_mode = iceberg::Mode::Campaign;
  iceberg::Language language = iceberg::Language::Polish;
  bool language_override = false;
  int district = 0, save_slot = 1;
  bool event_level = false, summit = false;
  int chapter = 1;
  bool mute = false, remote_audio = false;
  int relay_port = 0;
  std::string ghost_host, ghost_port = "27888", room = "default";
  bool seed_override = false;
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
    if (arg == "--campaign" || arg == "--expedition" || arg == "--trial") {
      args.adventure = args.explicit_adventure = true;
      args.adventure_mode = arg=="--campaign"?iceberg::Mode::Campaign:
        arg=="--trial"?iceberg::Mode::Trial:iceberg::Mode::Endless;
    } else if(arg=="--terminal-view") {
      args.terminal_view=true;
    } else if(arg=="--diagnostic" || arg=="--legacy") {
      args.adventure=false;
    } else if(arg=="--language" && i+1<argc) {
      std::string value=argv[++i];
      if(value!="pl"&&value!="en"){std::cerr<<"Language: pl or en\n";std::exit(2);}
      args.language=value=="en"?iceberg::Language::English:iceberg::Language::Polish;
      args.language_override=true;
    } else if(arg=="--district" && i+1<argc) {
      std::string value=argv[++i];
      if(value.size()!=1||value[0]<'1'||value[0]>'6'){std::cerr<<"District: 1..6\n";std::exit(2);}
      args.district=value[0]-'1';args.district_override=true;
    } else if(arg=="--save-slot") {
      const std::string slot=i+1<argc?argv[++i]:"";
      if(slot.size()!=1||slot[0]<'1'||slot[0]>'3'){std::cerr<<"Save slot: 1..3\n";std::exit(2);}
      args.save_slot=slot[0]-'0';
    } else if (arg == "--event") {
      args.adventure=false;
      args.event_level = true;
      args.summit = false;
      args.chapter = 0;
    } else if (arg == "--summit") {
      args.adventure=false;
      args.summit = true;
      args.event_level = false;
      args.chapter = 0;
    } else if (arg == "--chapter" && i + 1 < argc) {
      args.adventure=false;
      std::string value = argv[++i];
      if (value != "1" && value != "2" && value != "3") {
        std::cerr << "Chapter must be 1..3\n";
        std::exit(2);
      }
      args.chapter = std::stoi(value);
      args.event_level = args.summit = false;
    } else if (arg == "--mute") {
      args.mute = true;
    } else if (arg == "--audio") {
      args.remote_audio = true;
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
        args.seed = uint32_t(v);args.seed_override=true;
        if(!args.explicit_adventure)args.adventure=false;
        args.chapter = 0;
        args.summit = args.event_level = false;
      } catch (...) {
        std::cerr << "Seed must be 0..4294967295\n";
        std::exit(2);
      }
    } else if (arg == "--ssh")
      args.input_mode = InputMode::Ssh;
    else if (arg == "--ssh-basic")
      args.input_mode = InputMode::SshBasic;
    else if (arg == "--demo") {
      args.demo = true;args.adventure=false;
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
          << "usage: terminal_velocity [--smoke] [--input-test] "
             "[--smoke-steps "
             "N] [--cell-aspect R] [--input evdev|terminal|auto] [--event] "
             "[--seed N] [--summit] [--chapter 1..3] [--mute] [--ssh | "
             "--ssh-basic]\n"
             "  --audio               Enable server-local audio in SSH mode\n"
             "  --campaign            Iceberg story (default)\n"
             "  --expedition          Endless iceberg routes\n"
             "  --trial               Eight-room mastery trial\n"
             "  --district 1..6       Expedition/trial district\n"
             "  --language pl|en      Polish (default) or English\n"
             "  --terminal-view       SDL: same Unicode cell view as terminal\n"
             "  --diagnostic          SDL: original graphical diagnostic view\n"
             "  --relay PORT          Run a headless UDP ghost relay\n"
             "  --connect HOST[:PORT] Join live ghosts (default port 27888)\n"
             "  --room NAME           Match friends in the same "
             "course/seed\n";
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

iceberg::SessionOptions adventure_options(const Args &args) {
  iceberg::SessionOptions o;
  o.seed=args.seed;o.seed_override=args.seed_override;o.language=args.language;o.language_override=args.language_override;
  o.mode=args.adventure_mode;o.district=args.district;o.district_override=args.district_override;o.mute=args.mute;
  o.ghost_host=args.ghost_host;o.ghost_port=args.ghost_port;o.ghost_room=args.room;
  o.remote_audio=args.remote_audio;o.save_slot=args.save_slot;
  o.remote=args.input_mode==InputMode::Ssh||args.input_mode==InputMode::SshBasic;
  return o;
}
[[maybe_unused]] int run_adventure(const Args &args,double aspect) {
  TerminalGuard terminal(args.input_mode==InputMode::Ssh);
  if(!terminal.active()){std::cerr<<"Run in a terminal, or use terminal_velocity_sdl --terminal-view\n";return 1;}
  TuiInput input(args.input_mode);
  if(!input.ready()){std::cerr<<"Input unavailable. Use ./play.sh or --ssh.\n";return 1;}
  AdventureSession session(adventure_options(args));
  auto last=std::chrono::steady_clock::now(),render_at=last;
  while(session.running&&!stop_requested) {
    auto now=std::chrono::steady_clock::now();
    double dt=clamp(std::chrono::duration<double>(now-last).count(),0.,.1);last=now;
    auto size=term_size();
    Controls c=input.poll(dt);
    bool was_blocked=session.state.menu||session.state.paused||session.state.journal||session.state.adventure.map||session.state.adventure.dialogue.active()||session.state.adventure.inspecting;
    session.state.ssh_basic=input.basic_ssh();session.state.input_source=input.label();
    session.tick(c,dt,input.focused,size.cols,size.rows,aspect);
    bool blocked=session.state.menu||session.state.paused||session.state.journal||session.state.adventure.map||session.state.adventure.dialogue.active()||session.state.adventure.inspecting;
    if(was_blocked!=blocked)input.clear();
    if(now>=render_at) {
      RenderGrid(session.grid,session.state,size.rows,size.cols,aspect).draw();
      render_at=now+std::chrono::microseconds(session.options.remote?33333:16667);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  session.persist();
  return session.legacy_requested?10:0;
}
#endif

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
    if(args.adventure) {
      auto o=adventure_options(args);o.no_save=true;o.mute=true;
      AdventureSession session(o);
      session.state.menu=args.preview=="menu";
      session.state.journal=args.preview=="journal";
      session.state.won=args.preview=="win";
      if(args.preview=="dialogue")session.talk(iceberg::introduction(0));
      update_camera(session.state,session.grid,110,27,aspect,10);
      RenderGrid(session.grid,session.state,34,110,aspect).draw();
      return 0;
    }
    auto g = args.chapter       ? build_chapter(args.chapter)
             : args.summit      ? build_summit()
             : args.event_level ? build_level()
                                : build_procedural(args.seed);
    auto s = new_state(g);
    ProgressStore progress;
    attach_progress(s, progress);
    s.menu = args.preview == "menu";
    s.journal = args.preview == "journal";
    s.won = args.preview == "win";
    s.input_source = "evdev";
    update_camera(s, g, 110, 27, aspect, 10);
    RenderGrid(g, s, 34, 110, aspect).draw();
    return 0;
  }
  if(args.terminal_view) {
    auto app=audio_directory().parent_path().parent_path()/
#ifdef _WIN32
      "terminal_velocity_sdl.exe";
    std::vector<const char*> forwarded;auto executable=app.string();forwarded.push_back(executable.c_str());
    for(int i=1;i<argc;++i)forwarded.push_back(argv[i]);
    forwarded.push_back(nullptr);
    int result=int(::_spawnv(_P_WAIT,executable.c_str(),forwarded.data()));
    if(result>=0)return result;
#else
      "terminal_velocity_sdl";
    std::vector<char*> forwarded;auto executable=app.string();forwarded.push_back(executable.data());
    for(int i=1;i<argc;++i)forwarded.push_back(argv[i]);
    forwarded.push_back(nullptr);
    ::execv(executable.c_str(),forwarded.data());
#endif
    std::cerr<<"SDL cell view is unavailable. Build or restore terminal_velocity_sdl.\n";return 1;
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
    if(args.adventure) {
      int result=run_adventure(args,aspect);
      if(result!=10)return result;
    }
    return run_game(aspect, args.input_mode, args.demo, args.event_level,
                    args.seed, args.summit, args.ghost_host, args.ghost_port,
                    args.room, args.chapter, args.mute, args.remote_audio);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
#endif
