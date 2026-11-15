#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <deque>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

namespace {
constexpr int W = 1280, H = 720;
constexpr double PPU = 30.0;
struct RGBA {
  Uint8 r, g, b, a = 255;
};
const RGBA ink{25, 20, 24}, cream{250, 247, 239}, gold{244, 205, 94},
    red{220, 48, 70}, muted{192, 177, 174}, teal{244, 205, 94};
SDL_Renderer *ren = nullptr;
void color(RGBA c) { SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a); }
void box(double x, double y, double w, double h, RGBA c) {
  color(c);
  SDL_Rect r{int(std::floor(x)), int(std::floor(y)),
             int(std::ceil(x + w) - std::floor(x)),
             int(std::ceil(y + h) - std::floor(y))};
  SDL_RenderFillRect(ren, &r);
}
void line(double x, double y, double a, double b, RGBA c) {
  color(c);
  SDL_RenderDrawLineF(ren, float(x), float(y), float(a), float(b));
}
void circle(int x, int y, int r, RGBA c) {
  for (int dy = -r; dy <= r; ++dy) {
    int dx = int(std::sqrt(r * r - dy * dy));
    line(x - dx, y + dy, x + dx, y + dy, c);
  }
}
void tri(float x, float y, float w, float h, RGBA c) {
  SDL_Color sc{c.r, c.g, c.b, c.a};
  SDL_Vertex v[3] = {{{x, y + h}, sc, {0, 0}},
                     {{x + w / 2, y}, sc, {0, 0}},
                     {{x + w, y + h}, sc, {0, 0}}};
  SDL_RenderGeometry(ren, nullptr, v, 3, nullptr, 0);
}
std::vector<unsigned> utf8(const std::string &s) {
  std::vector<unsigned> out;
  for (size_t i = 0; i < s.size();) {
    unsigned c = (unsigned char)s[i++];
    int n = 0;
    if (c >= 240) {
      c &= 7;
      n = 3;
    } else if (c >= 224) {
      c &= 15;
      n = 2;
    } else if (c >= 192) {
      c &= 31;
      n = 1;
    }
    while (n-- && i < s.size())
      c = (c << 6) | ((unsigned char)s[i++] & 63);
    out.push_back(c);
  }
  return out;
}
class Font {
  FT_Library lib{};
  FT_Face face{};
  struct Glyph {
    SDL_Texture *texture = nullptr;
    int w = 0, h = 0, left = 0, top = 0, advance = 0;
  };
  std::map<std::pair<int, unsigned>, Glyph> cache;

public:
  bool init(const std::string &path) {
    return FT_Init_FreeType(&lib) == 0 &&
           FT_New_Face(lib, path.c_str(), 0, &face) == 0;
  }
  Glyph &glyph(int size, unsigned c) {
    auto key = std::make_pair(size, c);
    auto it = cache.find(key);
    if (it != cache.end())
      return it->second;
    FT_Set_Pixel_Sizes(face, 0, size);
    FT_Load_Char(face, c, FT_LOAD_RENDER);
    auto b = face->glyph;
    Glyph g;
    g.w = b->bitmap.width;
    g.h = b->bitmap.rows;
    g.left = b->bitmap_left;
    g.top = b->bitmap_top;
    g.advance = b->advance.x >> 6;
    if (g.w && g.h) {
      SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, g.w, g.h, 32,
                                                      SDL_PIXELFORMAT_RGBA32);
      for (int y = 0; y < g.h; ++y)
        for (int x = 0; x < g.w; ++x) {
          Uint8 alpha = b->bitmap.buffer[y * b->bitmap.pitch + x];
          auto *pixels = (Uint32 *)((Uint8 *)s->pixels + y * s->pitch);
          pixels[x] = SDL_MapRGBA(s->format, 255, 255, 255, alpha);
        }
      g.texture = SDL_CreateTextureFromSurface(ren, s);
      SDL_FreeSurface(s);
      SDL_SetTextureBlendMode(g.texture, SDL_BLENDMODE_BLEND);
    }
    return cache.emplace(key, g).first->second;
  }
  int width(const std::string &s, int size) {
    int w = 0;
    for (auto c : utf8(s))
      w += glyph(size, c).advance;
    return w;
  }
  void text(const std::string &s, int x, int y, int size, RGBA c = cream) {
    for (auto ch : utf8(s)) {
      auto &g = glyph(size, ch);
      if (g.texture) {
        SDL_SetTextureColorMod(g.texture, c.r, c.g, c.b);
        SDL_SetTextureAlphaMod(g.texture, c.a);
        SDL_Rect r{x + g.left, y + size - g.top, g.w, g.h};
        SDL_RenderCopy(ren, g.texture, nullptr, &r);
      }
      x += g.advance;
    }
  }
  void center(const std::string &s, int y, int size, RGBA c = cream) {
    text(s, (W - width(s, size)) / 2, y, size, c);
  }
  ~Font() {
    for (auto &[key, g] : cache)
      if (g.texture)
        SDL_DestroyTexture(g.texture);
    if (face)
      FT_Done_Face(face);
    if (lib)
      FT_Done_FreeType(lib);
  }
};
struct Sound {
  SDL_AudioDeviceID device = 0;
  bool muted = false;
  void init() {
    SDL_AudioSpec desired{};
    desired.freq = 48000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 512;
    device = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
    if (device)
      SDL_PauseAudioDevice(device, 0);
  }
  void play(int kind) {
    if (!device || muted)
      return;
    double freq = kind == 0   ? 440
                  : kind == 1 ? 660
                  : kind == 2 ? 880
                  : kind == 3 ? 180
                              : 523.25;
    double duration = kind == 3 ? .15 : kind == 4 ? .4 : .09;
    std::vector<float> wave(int(48000 * duration));
    for (size_t i = 0; i < wave.size(); ++i) {
      double t = double(i) / 48000, env = std::sin(3.1415926535 * t / duration);
      double f = freq * (kind == 0 ? 1 + t * 3 : kind == 3 ? 1 - t * 3 : 1);
      wave[i] = float(
          .08 * env *
          (std::sin(t * f * 6.2831853) + .25 * std::sin(t * f * 12.56637)));
    }
    SDL_ClearQueuedAudio(device);
    SDL_QueueAudio(device, wave.data(), wave.size() * sizeof(float));
  }
  ~Sound() {
    if (device)
      SDL_CloseAudioDevice(device);
  }
};
struct Sweet {
  double x, y;
  bool got = false;
};
struct Particle {
  double x, y, vx, vy, life;
  RGBA c;
};
struct Trail {
  double x, y, life;
};
struct App {
  EngineGrid grid = build_level();
  GameState state = new_state(grid);
  FixedStepper stepper;
  std::array<bool, SDL_NUM_SCANCODES> keys{};
  enum Mode { Menu, Play, Pause, Journal, Win } mode = Menu, return_mode = Menu;
  bool jump_edge = false, dash_edge = false, restart_edge = false,
       debug = false, running = true, fullscreen = false;
  bool focused = true;
  double time = 0, elapsed = 0, toast = 0, flash = 0, best = 0, input_age = 0,
         fps = 120;
  Uint32 last_event = 0;
  int chapter = 0, frames = 0;
  double camx = 0, camy = 17.5;
  std::vector<Sweet> sweets;
  std::vector<Particle> particles;
  std::deque<Trail> trail;
  std::string base;
  Sound sound;
  ghosts::Client network;
  std::string room = "default";
  uint32_t network_epoch = 0;
  explicit App(std::string p, bool event_level = true, uint32_t seed = 1,
               bool summit = false)
      : base(std::move(p)) {
    grid = summit        ? build_summit()
           : event_level ? build_level()
                         : build_procedural(seed);
    reset();
    std::ifstream f(base + "best.txt");
    if (grid.event_level)
      f >> best;
  }
  void reset() {
    ++network_epoch;
    state = new_state(grid);
    stepper = FixedStepper{};
    elapsed = 0;
    camx = 0;
    camy = 17.5;
    chapter = 0;
    toast = 0;
    flash = 0;
    particles.clear();
    trail.clear();
    keys.fill(false);
    jump_edge = dash_edge = restart_edge = false;
    sweets.clear();
    for (auto p : grid.sweets)
      sweets.push_back({p.first, p.second, false});
  }
  int count() const {
    int c = 0;
    for (auto s : sweets)
      c += s.got;
    return c;
  }
  bool down(SDL_Scancode s) const { return keys[s]; }
  Controls controls() const {
    Controls c;
    c.move = int(down(SDL_SCANCODE_D) || down(SDL_SCANCODE_RIGHT)) -
             int(down(SDL_SCANCODE_A) || down(SDL_SCANCODE_LEFT));
    c.jump_held = down(SDL_SCANCODE_SPACE) || down(SDL_SCANCODE_W) ||
                  down(SDL_SCANCODE_UP);
    c.jump_pressed = jump_edge;
    c.dash_pressed = dash_edge;
    c.restart = restart_edge;
    c.fast_fall = down(SDL_SCANCODE_S) || down(SDL_SCANCODE_DOWN);
    c.aim_y =
        int(c.fast_fall) - int(down(SDL_SCANCODE_W) || down(SDL_SCANCODE_UP));
    return c;
  }
  void clear_input() {
    keys.fill(false);
    jump_edge = dash_edge = restart_edge = false;
    stepper = FixedStepper{};
  }
  void event(const SDL_Event &e, SDL_Window *window) {
    if (e.type == SDL_QUIT) {
      running = false;
      return;
    }
    if (e.type == SDL_WINDOWEVENT &&
        e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
      focused = false;
      if (mode == Play)
        mode = Pause;
      clear_input();
    }
    if (e.type == SDL_WINDOWEVENT &&
        e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
      focused = true;
    if (e.type == SDL_KEYUP) {
      keys[e.key.keysym.scancode] = false;
      return;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
      float x, y;
      SDL_RenderWindowToLogical(ren, e.button.x, e.button.y, &x, &y);
      if (mode == Menu && x >= 72 && x <= 398 && y >= 454 && y <= 516) {
        mode = Play;
        clear_input();
        sound.play(4);
      }
      if (mode == Menu && x >= 72 && x <= 398 && y >= 530 && y <= 580) {
        return_mode = Menu;
        mode = Journal;
      }
      return;
    }
    if (e.type != SDL_KEYDOWN || e.key.repeat || !focused)
      return;
    auto k = e.key.keysym.scancode;
    if (k == SDL_SCANCODE_M && mode != Play) {
      sound.muted = !sound.muted;
      return;
    }
    if ((mode == Menu || mode == Pause || mode == Win) &&
        (k == SDL_SCANCODE_1 || k == SDL_SCANCODE_2 || k == SDL_SCANCODE_3 ||
         k == SDL_SCANCODE_G)) {
      uint32_t seed = grid.seed + (k == SDL_SCANCODE_G ? 1 : 0);
      grid = k == SDL_SCANCODE_3   ? build_summit()
             : k == SDL_SCANCODE_1 ? build_level()
                                   : build_procedural(seed);
      reset();
      mode = Menu;
      best = 0;
      return;
    }
    keys[k] = true;
    last_event = SDL_GetTicks();
    if (k == SDL_SCANCODE_F3) {
      debug = !debug;
      return;
    }
    if (k == SDL_SCANCODE_M) {
      sound.muted = !sound.muted;
      return;
    }
    if (k == SDL_SCANCODE_F11) {
      fullscreen = !fullscreen;
      SDL_SetWindowFullscreen(window,
                              fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
      return;
    }
    if (k == SDL_SCANCODE_Q && mode != Play) {
      running = false;
      return;
    }
    if (k == SDL_SCANCODE_TAB) {
      if (mode == Journal)
        mode = return_mode;
      else {
        return_mode = mode;
        mode = Journal;
      }
      clear_input();
      return;
    }
    if (k == SDL_SCANCODE_ESCAPE) {
      if (mode == Journal)
        mode = return_mode;
      else if (mode == Play)
        mode = Pause;
      else if (mode == Pause)
        mode = Play;
      else if (mode == Win)
        mode = Menu;
      clear_input();
      return;
    }
    if ((k == SDL_SCANCODE_RETURN || k == SDL_SCANCODE_SPACE) && mode != Play) {
      if (mode == Menu || mode == Win) {
        reset();
        mode = Play;
        sound.play(4);
      } else if (mode == Pause)
        mode = Play;
      else
        mode = return_mode;
      clear_input();
      return;
    }
    if (k == SDL_SCANCODE_R && mode != Journal) {
      if (down(SDL_SCANCODE_LCTRL) || down(SDL_SCANCODE_RCTRL) || mode == Win) {
        reset();
        mode = Play;
      } else if (mode == Play)
        restart_edge = true;
      return;
    }
    if (mode != Play)
      return;
    if (k == SDL_SCANCODE_SPACE || k == SDL_SCANCODE_W || k == SDL_SCANCODE_UP)
      jump_edge = true;
    if (k == SDL_SCANCODE_X || k == SDL_SCANCODE_LSHIFT ||
        k == SDL_SCANCODE_RSHIFT)
      dash_edge = true;
  }
  void burst(double x, double y, RGBA c, int n) {
    for (int i = 0; i < n; ++i) {
      double a = i * 2.399963;
      particles.push_back({x, y, std::cos(a) * (1 + i % 5) * 1.5,
                           std::sin(a) * (1 + i % 5) * 1.5, .3 + (i % 7) * .06,
                           c});
    }
  }
  void network_tick(double dt) {
    state.multiplayer = network.enabled();
    ghosts::Snapshot s;
    s.room = room;
    s.level = grid.summit ? 2 : grid.event_level ? 0 : 1;
    s.seed = grid.summit || grid.event_level ? 0 : grid.seed;
    s.x = state.player.x;
    s.y = state.player.y;
    s.epoch = network_epoch;
    s.flags = (state.player.dash_available ? 1 : 0) |
              (mode == Play || mode == Win ? 2 : 0) | (state.won ? 4 : 0);
    network.tick(s, dt);
    state.ghosts = network.visible();
  }
  void tick(double dt) {
    time += dt;
    if (mode != Play)
      return;
    elapsed = state.elapsed;
    toast = std::max(0.0, toast - dt);
    flash = std::max(0.0, flash - dt);
    Controls c = controls();
    int deaths = state.deaths, cp = state.checkpoint;
    bool wasground = state.player.grounded;
    double vy = state.player.vy, oldx = state.player.x, oldy = state.player.y;
    bool hadpending = stepper.pending.jump_pressed ||
                      stepper.pending.dash_pressed || jump_edge || dash_edge;
    stepper.advance(state, grid, c, dt);
    elapsed = state.elapsed;
    if (hadpending && !stepper.pending.jump_pressed &&
        !stepper.pending.dash_pressed)
      input_age = SDL_GetTicks() - last_event;
    jump_edge = dash_edge = restart_edge = false;
    if (c.jump_pressed && state.player.vy < 0)
      sound.play(0);
    if (c.dash_pressed && state.player.dash_time_left > 0)
      sound.play(1);
    if (state.deaths != deaths || c.restart) {
      ++network_epoch;
      burst(oldx, oldy, red, 24);
      sound.play(3);
      flash = .16;
      camx = std::max(0.0, state.player.x - 13);
      trail.clear();
    }
    if (!wasground && state.player.grounded && vy > 9)
      burst(state.player.x, 38, cream, 9);
    if (cp != state.checkpoint) {
      toast = 5;
      sound.play(4);
      burst(state.player.x, state.player.y, gold, 30);
    }
    chapter = state.player.x < 60 ? 0 : state.player.x < 118 ? 1 : 2;
    for (size_t i = 0; i < sweets.size(); ++i)
      if (!sweets[i].got && state.collected[i]) {
        sweets[i].got = true;
        burst(sweets[i].x, sweets[i].y, gold, 15);
        sound.play(2);
      }
    if (state.player.dash_time_left > 0)
      trail.push_back({state.player.x, state.player.y, .18});
    for (auto &t : trail)
      t.life -= dt;
    while (!trail.empty() && trail.front().life <= 0)
      trail.pop_front();
    for (auto &p : particles) {
      p.life -= dt;
      p.x += p.vx * dt;
      p.y += p.vy * dt;
      p.vy += 10 * dt;
    }
    std::erase_if(particles, [](auto p) { return p.life <= 0; });
    double target = clamp(state.player.x - 14 + state.player.vx * .12, 0.0,
                          grid.width() - W / PPU);
    camx += (target - camx) * (1 - std::exp(-dt * 10));
    if (state.won) {
      mode = Win;
      sound.play(4);
      if (best <= 0 || elapsed < best) {
        best = elapsed;
        std::ofstream f(grid.event_level ? base + "best.txt" : "/dev/null");
        f << best;
      }
      clear_input();
    }
  }
  double sx(double x) const { return (x - camx) * PPU; }
  double sy(double y) const { return (y - camy) * PPU; }
};
std::string clock_text(double sec) {
  int m = int(sec) / 60;
  double s = sec - m * 60;
  std::ostringstream o;
  o << m << ":" << std::setfill('0') << std::setw(5) << std::fixed
    << std::setprecision(2) << s;
  return o.str();
}
void flag(double x, double y, double scale, double t) {
  box(x, y, 2 * scale, 49 * scale, gold);
  for (int i = 0; i < 28; ++i) {
    double dy = std::sin(t * 3 + i * .22) * 2 * scale;
    box(x + (i + 2) * scale, y + dy, scale + 1, 9 * scale, cream);
    box(x + (i + 2) * scale, y + 9 * scale + dy, scale + 1, 9 * scale, red);
  }
}
void pope(double x, double y, double scale, bool ready, int facing, double t,
          bool moving) {
  // Original pixel illustration. White cassock, zucchetto, gold stole, red
  // shoes.
  const char *rows[] = {
      "....WWWW....", "...WWWWWW...", "...SSSSSS...", "...SKSSKS...",
      "....SSSS....", "...WWGGWW...", "..WWWGGWWW..", ".WWWWGGWWWW.",
      "WWWWWGGWWWWW", "WWWWWGGWWWWW", ".WWWWGGWWWW.", "..WWWGGWWW..",
      "..WWWGGWWW..", "..WWWWWWWW..", "..WWWWWWWW..", "..WWWWWWWW..",
      "..WWWWWWWW..", "..WWWWWWWW..", "..RR..RR....", "..RR..RR...."};
  double bob = moving ? std::sin(t * 18) * scale * .7 : 0;
  for (int j = 0; j < 20; ++j)
    for (int i = 0; i < 12; ++i) {
      char ch = rows[j][i];
      if (ch == '.')
        continue;
      RGBA c = ch == 'W'   ? (ready ? cream : muted)
               : ch == 'G' ? gold
               : ch == 'S' ? RGBA{232, 176, 140}
               : ch == 'R' ? red
                           : ink;
      double dx = (facing > 0 ? i : 11 - i) * scale;
      box(x + dx, y + j * scale + bob, scale + .2, scale + .2, c);
    }
}
void skyline(const App &a) {
  for (int y = 0; y < H; ++y) {
    double f = double(y) / H;
    line(0, y, W, y,
         {Uint8(25 + f * 21), Uint8(20 + f * 9), Uint8(27 + f * 12)});
  }
  for (int i = 0; i < 65; ++i) {
    int x = (i * 193 + 37) % 1280, y = (i * 79 + 17) % 330;
    box(x, y, i % 3 == 0 ? 2 : 1, 2,
        {190, 189, 165, Uint8(70 + 40 * std::sin(a.time + i))});
  }
  circle(1045, 139, 49, {239, 205, 139});
  circle(1063, 125, 42, {29, 22, 29});
  for (int layer = 0; layer < 2; ++layer) {
    double drift = a.camx * (layer ? 8 : 3);
    for (int i = -2; i < 24; ++i) {
      double x = i * 95 - std::fmod(drift, 95.0);
      int h = 60 + (i + 20) * 37 % 130;
      double y = 540 - h + layer * 45;
      RGBA c = layer ? RGBA{57, 35, 43} : RGBA{43, 29, 38};
      box(x, y, 86, H - y, c);
      tri(x - 6, y - 33, 98, 34, c);
      if (i % 5 == 0) {
        box(x + 28, y - 98, 30, 100, c);
        tri(x + 22, y - 135, 42, 39, c);
        line(x + 43, y - 154, x + 43, y - 131, gold);
        line(x + 37, y - 145, x + 49, y - 145, gold);
      }
      if (layer)
        for (int w = 0; w < 3; ++w)
          for (int r = 0; r < 3; ++r)
            box(x + 13 + w * 24, y + 20 + r * 30, 8, 13,
                {203, 165, 94, Uint8((i + w + r) % 3 == 0 ? 100 : 30)});
    }
  }
  for (int i = 0; i < 15; ++i) {
    double x = std::fmod(i * 97 + a.time * (8 + i % 4), 1320.0) - 20;
    double y = std::fmod(i * 51 + a.time * (12 + i % 3), 600.0);
    line(x, y, x + 4, y + 2, {239, 193, 105, 70});
  }
}
void world(App &a, Font &f) {
  skyline(a);
  for (int row = 28; row < 46; ++row)
    for (int col = std::max(0, int(a.camx) - 1);
         col < std::min(a.grid.width(), int(a.camx + W / PPU) + 2); ++col) {
      double x = a.sx(col), y = a.sy(row);
      if (a.grid.solid_at(col, row)) {
        bool top = !a.grid.solid_at(col, row - 1);
        box(x, y, PPU + 1, PPU + 1, {82, 43, 54});
        box(x + 1, y + 2, PPU - 2, PPU - 3,
            {Uint8(99 + (col + row) % 3 * 4), Uint8(51 + (col + row) % 3 * 4),
             Uint8(62 + (col + row) % 3 * 4)});
        if (top) {
          box(x, y, PPU + 1, 5, cream);
          box(x, y + 5, PPU + 1, 4, red);
        } else
          line(x, y + PPU - 1, x + PPU, y + PPU - 1, {32, 43, 52});
      }
      if (a.grid.hazard_at(col, row)) {
        tri(x + 3, y + 11, 12, 19, red);
        tri(x + 16, y + 11, 12, 19, red);
      }
    }
  for (size_t cp = 0; cp < a.grid.checkpoints.size(); ++cp) {
    double px = a.grid.checkpoints[cp];
    flag(a.sx(px), a.sy(38) - 84, 1.4, a.time);
    if (cp > 0)
      f.text(int(cp) <= a.state.checkpoint ? "ZAPISANO" : "PUNKT ZAPISU",
             int(a.sx(px)) - 40, int(a.sy(38)) - 112, 13,
             int(cp) <= a.state.checkpoint ? teal : gold);
  }
  for (auto platform : a.grid.platforms) {
    auto r = platform.at(a.state.elapsed);
    box(a.sx(r.x), a.sy(r.y), r.w * PPU, r.h * PPU, gold);
    line(a.sx(r.x), a.sy(r.y) + 4, a.sx(r.right()), a.sy(r.y) + 4, cream);
  }
  for (size_t i = 0; i < a.grid.refills.size(); ++i) {
    auto [x, y] = a.grid.refills[i];
    auto c = a.state.refill_cooldown[i] > 0 ? muted : RGBA{93, 201, 210};
    tri(a.sx(x) - 9, a.sy(y) - 13, 18, 13, c);
    tri(a.sx(x) - 9, a.sy(y) + 13, 18, -13, c);
  }
  for (const auto &g : a.state.ghosts)
    box(a.sx(g.x), a.sy(g.y), kPlayerWidth * PPU, kPlayerHeight * PPU,
        {93, 201, 210, 110});
  for (auto r : a.grid.springs) {
    box(a.sx(r.x), a.sy(r.y), r.w * PPU, 8, teal);
    for (int n = 0; n < 3; ++n)
      tri(a.sx(r.x) + n * 12, a.sy(r.y) - 10, 10, 10, teal);
  }
  for (size_t i = 0; i < a.grid.enemies.size(); ++i)
    if (!a.state.defeated[i]) {
      auto r = a.grid.enemies[i].at(a.state.elapsed);
      box(a.sx(r.x), a.sy(r.y), r.w * PPU, r.h * PPU, red);
      box(a.sx(r.x) + 5, a.sy(r.y) + 6, 6, 7, cream);
      box(a.sx(r.x) + 22, a.sy(r.y) + 6, 6, 7, cream);
    }
  for (size_t i = 0; i < a.sweets.size(); ++i) {
    auto s = a.sweets[i];
    if (s.got)
      continue;
    double x = a.sx(s.x), y = a.sy(s.y) + std::sin(a.time * 3 + i) * 3;
    circle(int(x + 10), int(y + 8), 18, {239, 193, 105, 22});
    if (!a.grid.event_level) {
      tri(x, y - 5, 23, 15, gold);
      tri(x, y + 9, 23, -15, cream);
      continue;
    }
    box(x, y, 23, 5, gold);
    box(x, y + 5, 23, 8, cream);
    box(x, y + 13, 23, 4, gold);
    box(x + 3, y - 2, 17, 3, {255, 251, 234});
  }
  double gx = a.sx(a.grid.goal.x + 1), gy = a.sy(38);
  box(gx - 40, gy - 175, 8, 175, gold);
  box(gx + 55, gy - 175, 8, 175, gold);
  box(gx - 40, gy - 175, 103, 13, cream);
  box(gx - 40, gy - 162, 103, 8, red);
  f.text("META", int(gx) - 17, int(gy) - 147, 19, gold);
  flag(gx, gy - 235, 1.7, a.time);
  for (auto t : a.trail)
    box(a.sx(t.x), a.sy(t.y), kPlayerWidth * PPU, kPlayerHeight * PPU,
        {239, 193, 105, Uint8(clamp(t.life / .18 * 130, 0, 255))});
  auto &p = a.state.player;
  box(a.sx(p.x) - 3, a.sy(p.y + kPlayerHeight) + 3, 32, 4, {0, 0, 0, 75});
  if (a.grid.event_level)
    pope(a.sx(p.x) - 2, a.sy(p.y), 2.65, p.dash_available, p.facing, a.time,
         p.grounded && std::abs(p.vx) > 1);
  else {
    double x = a.sx(p.x), y = a.sy(p.y);
    auto body = p.dash_available ? gold : cream;
    box(x, y, 27, 15, body);
    box(x + 4, y + 6, 19, 5, ink);
    box(x + 3, y + 17, 21, 27, p.dash_available ? gold : muted);
    box(x + 2, y + 44, 9, 10, body);
    box(x + 17, y + 44, 9, 10, body);
  }
  for (auto p : a.particles)
    box(a.sx(p.x), a.sy(p.y), 4, 4,
        {p.c.r, p.c.g, p.c.b, Uint8(std::min(1.0, p.life * 3) * 255)});
  if (a.debug) {
    color(teal);
    SDL_FRect r{float(a.sx(p.x)), float(a.sy(p.y)), float(kPlayerWidth * PPU),
                float(kPlayerHeight * PPU)};
    SDL_RenderDrawRectF(ren, &r);
    line(r.x + 13, r.y + 27, r.x + 13 + p.vx * 3, r.y + 27 + p.vy * 3, gold);
    for (int x = int(a.camx); x < a.camx + 43; ++x)
      line(a.sx(x), 100, a.sx(x), 650, {109, 214, 187, 30});
  }
  if (a.flash > 0)
    box(0, 0, W, H, {207, 59, 77, Uint8(a.flash * 450)});
}
void hud(App &a, Font &f) {
  box(0, 0, W, 91, {25, 20, 24, 240});
  box(32, 28, 4, 36, red);
  f.text(a.grid.event_level ? "EVENT: SZTAFETA WOLNOŚCI" : "TERMINAL VELOCITY",
         48, 24, 21);
  const char *chapters[] = {"01 / WADOWICE • ROZGRZEWKA",
                            "02 / KRAKÓW • NABIERZ PĘDU",
                            "03 / WARSZAWA • DO WOLNOŚCI"};
  f.text(a.grid.event_level ? chapters[a.chapter]
         : a.grid.summit ? "SZCZYT / PRECYZJA"
                         : "EKSPEDYCJA • SEED " + std::to_string(a.grid.seed),
         48, 53, 12, gold);
  f.text(a.grid.event_level ? "KREMÓWKI" : "KRYSZTAŁY", 495, 21, 11, muted);
  f.text(std::to_string(a.count()) + " / " +
             std::to_string(a.grid.sweets.size()),
         495, 40, 24, gold);
  f.text("ZRYW / SHIFT", 667, 21, 11, muted);
  f.text(a.state.player.dash_available ? "GOTOWY" : "WYLĄDUJ", 667, 43, 18,
         a.state.player.dash_available ? teal : muted);
  f.text("CZAS", 869, 21, 11, muted);
  f.text(clock_text(a.elapsed), 869, 40, 24);
  f.text("PRÓBY", 1086, 21, 11, muted);
  f.text(std::to_string(a.state.deaths + 1), 1086, 40, 24);
  box(32, 86, 1216, 2, {48, 64, 73});
  box(32, 86, 1216 * clamp(a.state.player.x / a.grid.goal.x, 0, 1), 2, gold);
  box(0, 665, W, 55, {25, 20, 24, 240});
  f.text("A D  Ruch     SPACJA  Skok     SHIFT + kierunek  Zryw     R  Ponów",
         32, 684, 14, cream);
  f.text("TAB  Pomoc     ESC  Pauza", 954, 684, 14, gold);
  if (a.elapsed < 10 && a.state.player.x < 20) {
    box(300, 138, 680, 79, {25, 20, 24, 225});
    f.center("Przytrzymaj skok, by polecieć wyżej. Puść, by skoczyć krótko.",
             151, 17);
    f.center(a.grid.event_level
                 ? "Złap kremówkę lub wyląduj, aby odzyskać zryw."
                 : "Złap kryształ lub wyląduj, aby odzyskać zryw.",
             180, 15, gold);
  }
  if (a.toast > 0) {
    box(340, 132, 600, 90, {25, 20, 24, 240});
    f.center("PUNKT ZAPISU • ŚWIETNIE IDZIE!", 148, 22, teal);
    f.center("R wraca tutaj. TAB otwiera dziennik niepodległości.", 183, 16);
  }
  if (a.debug) {
    box(32, 110, 350, 187, {8, 15, 24, 235});
    f.text("F3 • DIAGNOSTYKA", 48, 121, 17, teal);
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << "FPS " << a.fps
      << "   Fizyka: 120 Hz";
    f.text(o.str(), 48, 151, 14);
    o.str("");
    o << "vx " << a.state.player.vx << "   vy " << a.state.player.vy;
    f.text(o.str(), 48, 177, 14);
    f.text("Zdarzenie → tick: " + std::to_string(int(a.input_age)) + " ms", 48,
           203, 14);
    auto c = a.controls();
    f.text("Ruch " + std::to_string(c.move) + "   Skok " +
               std::to_string(c.jump_held) + "   Dół " +
               std::to_string(c.fast_fall),
           48, 229, 14);
    f.text("SDL keydown / keyup • poza terminalem", 48, 258, 13, gold);
  }
}
void menu(App &a, Font &f) {
  if (!a.grid.event_level) {
    box(0, 0, W, H, {17, 19, 28, 235});
    f.center("TERMINAL VELOCITY", 98, 54, gold);
    f.center(a.grid.summit ? "SZCZYT / PRECYZJA" : "EKSPEDYCJA PROCEDURALNA",
             184, 28);
    f.center(a.grid.summit ? "6 etapów / skoki / zrywy / odnowienia"
                           : "Seed " + std::to_string(a.grid.seed),
             238, 22, gold);
    f.center("[1] Event JP2 / 11 listopada     [2] Ekspedycja     [3] Szczyt   "
             "  [G] Nowy seed",
             322, 22);
    f.center("ENTER — GRAJ", 400, 32, gold);
    f.center("Ruchome platformy • sprężyny • patrole • opcjonalne górne trasy",
             479, 20);
    f.center("Skocz na wroga lub pokonaj go zrywem. S schodzi z platformy.",
             522, 19, muted);
    f.center("GUI DO DEBUGOWANIA • główna gra działa w Ghostty", 622, 18,
             muted);
    return;
  }

  box(0, 0, W, H, {21, 17, 23, 160});
  box(0, 0, 730, H, {27, 19, 25, 235});
  box(72, 65, 40, 5, cream);
  box(112, 65, 40, 5, red);
  f.text("[1] EVENT 11 XI   [2] EKSPEDYCJA   [3] SZCZYT   [G] NOWY SEED", 72,
         94, 14, gold);
  f.text("JP2", 67, 128, 105, cream);
  f.text("SZTAFETA", 70, 253, 61, cream);
  f.text("WOLNOŚCI", 70, 320, 61, cream);
  f.text("Z Wadowic do Warszawy. Z kremówką w dłoni.", 74, 407, 19, muted);
  box(72, 454, 326, 62, red);
  f.text("RUSZAMY   →   ENTER", 100, 471, 20, cream);
  box(72, 530, 326, 49, {60, 34, 43});
  f.text("DZIENNIK NIEPODLEGŁOŚCI   TAB", 92, 545, 14, gold);
  f.text("3 odcinki  /  15 kremówek  /  jedna biało-czerwona", 74, 607, 15,
         muted);
  f.text("A D • ruch    SPACJA • skok    SHIFT • zryw", 74, 640, 14, cream);
  f.text("M • dźwięk    F11 • pełny ekran    Q • wyjście", 74, 672, 12, muted);
  circle(1003, 317, 151, {239, 193, 105, 20});
  circle(1003, 317, 120, {239, 193, 105, 20});
  pope(946, 201, 10, true, 1, a.time, false);
  flag(1111, 192, 2.4, a.time);
  f.text("11", 880, 470, 68, gold);
  f.text("LISTOPADA", 984, 486, 23, cream);
  f.text("PAMIĘĆ • WOLNOŚĆ • ZABAWA", 867, 565, 13, gold);
  f.text("Żartobliwa sztafeta przez polską historię.", 850, 598, 14, muted);
  f.text("JP2 nie jest postacią wydarzeń z 1918 r.", 850, 622, 14, muted);
}
void journal(Font &f, bool event_level = true) {
  if (!event_level) {
    box(0, 0, W, H, {21, 17, 23, 249});
    f.center("EKSPEDYCJA / MECHANIKI", 100, 36, gold);
    f.center("=== Windy: wskakuj od dołu. S pozwala zejść przez platformę.",
             220, 23);
    f.center("^^ Sprężyny: wysoki skok i odnowienie zrywu.", 285, 23);
    f.center("[!] Patrole: skocz na głowę albo użyj zrywu. Unikaj boków.", 350,
             23);
    f.center("N / Ctrl+R: ponów seed. 1 / 2 / G w menu: wybierz trasę.", 415,
             21);
    f.center("TAB / ESC — powrót", 590, 23, gold);
    return;
  }

  box(0, 0, W, H, {21, 17, 23, 249});
  box(56, 48, 4, 50, red);
  f.text("DZIENNIK NIEPODLEGŁOŚCI", 76, 42, 32);
  f.text("Gra jest fantazją. Te daty są prawdziwe.", 77, 88, 17, gold);
  const char *dates[] = {"1795 → 1918", "11 XI 1918", "1920 → 1978"};
  const char *titles[] = {"123 lata zaborów", "Odrodzenie państwa",
                          "Jan Paweł II"};
  const std::vector<std::vector<std::string>> copy = {
      {"W 1795 roku trzeci rozbiór zakończył", "istnienie I Rzeczypospolitej.",
       "Przez kolejne pokolenia Polacy", "podtrzymywali język, kulturę",
       "i dążenie do własnego państwa."},
      {"11 listopada Piłsudski przejął", "od Rady Regencyjnej władzę",
       "wojskową. Ta data symbolizuje", "odzyskanie niepodległości.",
       "Odbudowa Polski była procesem,", "a nie wydarzeniem jednego dnia."},
      {"Karol Wojtyła urodził się w 1920 r.", "w Wadowicach. Papieżem został",
       "16 października 1978 roku.", "W grze jest symbolicznym",
       "przewodnikiem po pamięci", "i polskiej kulturze — nie twórcą",
       "niepodległości w 1918 roku."}};
  for (int i = 0; i < 3; ++i) {
    int x = 56 + i * 397;
    box(x, 163, 377, 378, {48, 30, 37});
    box(x, 163, 377, 4, i == 1 ? red : gold);
    f.text(dates[i], x + 24, 189, 15, gold);
    f.text(titles[i], x + 24, 229, 23);
    for (size_t j = 0; j < copy[i].size(); ++j)
      f.text(copy[i][j], x + 24, 285 + int(j) * 29, 16, cream);
  }
  f.text("Źródła: Sejm RP — 11 listopada. Narodowe Święto Niepodległości; "
         "Watykan — biografia Jana Pawła II.",
         56, 574, 14, muted);
  f.text("Pełne odnośniki oraz opis projektu: plik README.md w folderze gry.",
         56, 601, 14, muted);
  f.center("TAB / ESC / ENTER    Wróć do gry", 658, 18, gold);
}
void pause_or_win(App &a, Font &f) {
  box(0, 0, W, H, {6, 14, 24, 210});
  box(310, 160, 660, 397, {43, 28, 35});
  box(310, 160, 660, 4, gold);
  if (a.mode == App::Pause) {
    f.center("CHWILA ODDECHU", 206, 38);
    f.center("ENTER / ESC   Wznów", 288, 22, gold);
    f.center("TAB   Dziennik niepodległości", 338, 18);
    f.center("M   Dźwięk: " +
                 std::string(a.sound.muted ? "wyłączony" : "włączony"),
             378, 18);
    f.center("F3   Diagnostyka     F11   Pełny ekran", 421, 16, muted);
    f.center("Q   Wyjście", 493, 16, muted);
  } else {
    f.center(a.grid.event_level ? "NIEPODLEGŁA!" : "EKSPEDYCJA UKOŃCZONA!", 190,
             45, gold);
    f.center(a.grid.event_level ? "Biało-czerwona dotarła do Warszawy."
                                : "Seed " + std::to_string(a.grid.seed),
             259, 20);
    f.center(clock_text(a.elapsed) + "   •   " + std::to_string(a.count()) +
                 " / " + std::to_string(a.grid.sweets.size()) + " znajdziek",
             309, 26);
    std::string medal = a.count() == int(a.grid.sweets.size())
                            ? "ZŁOTA KREMÓWKA"
                        : a.state.deaths == 0 ? "CZYSTY PRZELOT"
                                              : "SZTAFETA UKOŃCZONA";
    f.center(medal, 360, 19, teal);
    f.center("Rekord: " + clock_text(a.best) +
                 "   /   Próby: " + std::to_string(a.state.deaths + 1),
             400, 16, muted);
    f.center("ENTER   Jeszcze raz     TAB   Historia", 468, 20, gold);
    f.center("Cel dodatkowy: zbierz wszystkie znajdźki w jednym biegu.", 513,
             14, muted);
  }
}
} // namespace
int main(int argc, char **argv) {
  bool smoke = false;
  std::string screenshot, scene = "menu";
  int maxframes = 0;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--smoke")
      smoke = true;
    else if (arg == "--screenshot" && i + 1 < argc)
      screenshot = argv[++i];
    else if (arg == "--scene" && i + 1 < argc)
      scene = argv[++i];
    else if (arg == "--frames" && i + 1 < argc)
      maxframes = std::atoi(argv[++i]);
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER |
               SDL_INIT_AUDIO) != 0) {
    std::cerr << SDL_GetError() << '\n';
    return 1;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  SDL_Window *window =
      SDL_CreateWindow("Terminal Velocity • debug GUI", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, W, H,
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!window) {
    std::cerr << SDL_GetError() << '\n';
    SDL_Quit();
    return 1;
  }
  ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!ren)
    ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (!ren) {
    std::cerr << SDL_GetError() << '\n';
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_RenderSetLogicalSize(ren, W, H);
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
  char *bp = SDL_GetBasePath();
  std::string base = bp ? bp : "./";
  SDL_free(bp);
  int result = 0;
  {
    Font f;
    if (!f.init(base + "assets/font.ttf")) {
      std::cerr << "Missing assets/font.ttf\n";
      return 1;
    }
    const Args shared = parse_args(argc, argv, true);
    App a(base, shared.event_level, shared.seed, shared.summit);
    a.room = shared.room;
    if (shared.relay_port) {
      std::cerr << "Use terminal_velocity --relay PORT\n";
      return 2;
    }
    if (!shared.ghost_host.empty()) {
      try {
        a.network.connect_to(shared.ghost_host, shared.ghost_port);
      } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
      }
    }
    a.sound.init();
    if (scene == "play" || smoke)
      a.mode = App::Play;
    if (scene == "journal")
      a.mode = App::Journal;
    if (scene == "end") {
      a.mode = App::Win;
      a.elapsed = 95.4;
      a.best = 95.4;
      a.state.player.x = 177;
      a.camx = 141;
    }
    if (scene == "mid") {
      a.mode = App::Play;
      a.state.player.x = 88;
      a.state.player.y = 33.2;
      a.camx = 75;
      a.chapter = 1;
      a.elapsed = 42.5;
    }
    if (smoke) {
      // Real event handler: a press+release between simulation ticks must
      // survive.
      SDL_Event e{};
      e.type = SDL_KEYDOWN;
      e.key.keysym.scancode = SDL_SCANCODE_SPACE;
      a.event(e, window);
      e.type = SDL_KEYUP;
      a.event(e, window);
      a.tick(.001);
      if (!a.stepper.pending.jump_pressed) {
        std::cerr << "Lost short input edge\n";
        result = 2;
      }
      a.tick(.02);
      e.type = SDL_WINDOWEVENT;
      e.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
      a.event(e, window);
      if (a.mode != App::Pause || a.controls().jump_held) {
        std::cerr << "Focus loss failure\n";
        result = 3;
      }
      e.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
      a.event(e, window);
      a.reset();
      a.mode = App::Play;
      a.keys[SDL_SCANCODE_D] = true;
    }
    auto last = std::chrono::steady_clock::now();
    auto render_at = last;
    auto last_render = last;
    int frames = 0;
    while (a.running) {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        if (!smoke && screenshot.empty())
          a.event(e, window);
      }
      auto now = std::chrono::steady_clock::now();
      double dt = std::chrono::duration<double>(now - last).count();
      last = now;
      a.tick(smoke ? 1.0 / 120 : std::min(.1, dt));
      a.network_tick(std::min(.1, dt));
      if (now >= render_at || smoke || !screenshot.empty()) {
        double render_dt =
            std::chrono::duration<double>(now - last_render).count();
        last_render = now;
        if (render_dt > .001)
          a.fps += (1 / render_dt - a.fps) * .1;
        world(a, f);
        if (a.mode == App::Menu)
          menu(a, f);
        else {
          hud(a, f);
          if (a.mode == App::Journal)
            journal(f, a.grid.event_level);
          else if (a.mode == App::Pause || a.mode == App::Win)
            pause_or_win(a, f);
        }
        if (!screenshot.empty()) {
          int w, h;
          SDL_GetRendererOutputSize(ren, &w, &h);
          SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(
              0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
          if (SDL_RenderReadPixels(ren, nullptr, s->format->format, s->pixels,
                                   s->pitch) != 0 ||
              SDL_SaveBMP(s, screenshot.c_str()) != 0) {
            std::cerr << SDL_GetError() << '\n';
            result = 4;
          }
          SDL_FreeSurface(s);
          a.running = false;
        }
        SDL_RenderPresent(ren);
        render_at = now + std::chrono::microseconds(8333);
        ++frames;
        if ((smoke && frames >= 90) || (maxframes && frames >= maxframes))
          a.running = false;
      }
      if (!smoke)
        SDL_Delay(1);
    }
    if (smoke) {
      if (a.state.player.x < 8) {
        std::cerr << "Held movement failure\n";
        result = 5;
      }
      std::cout << "SDL smoke: key edges, focus pause, held input, renderer; x="
                << a.state.player.x << " result=" << result << '\n';
    }
  }
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return result;
}
