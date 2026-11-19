#pragma once
// Included after Controls/TermSize/request_stop. Windows.h comes from
// net_platform.hpp before any project namespace is opened.
class TerminalGuard {
  HANDLE input_ = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE output_ = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD input_mode_ = 0, output_mode_ = 0;
  UINT input_cp_ = 0, output_cp_ = 0;
  bool active_ = false;

public:
  explicit TerminalGuard(bool = false) {
    if (!GetConsoleMode(input_, &input_mode_) ||
        !GetConsoleMode(output_, &output_mode_))
      return;
    if (!SetConsoleMode(output_,
                        output_mode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
      return;
    if (!SetConsoleMode(input_, ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS)) {
      SetConsoleMode(output_, output_mode_);
      return;
    }
    input_cp_ = GetConsoleCP();
    output_cp_ = GetConsoleOutputCP();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    active_ = true;
    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);
    std::cout << "\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[2J\x1b[H" << std::flush;
  }
  TerminalGuard(const TerminalGuard &) = delete;
  TerminalGuard &operator=(const TerminalGuard &) = delete;
  ~TerminalGuard() {
    if (!active_)
      return;
    std::cout << "\x1b[?2026l\x1b[0m\x1b[?7h\x1b[?25h\x1b[?1049l" << std::flush;
    SetConsoleMode(input_, input_mode_);
    SetConsoleMode(output_, output_mode_);
    SetConsoleCP(input_cp_);
    SetConsoleOutputCP(output_cp_);
  }
  bool active() const { return active_; }
};

class TuiInput {
  HANDLE input_ = GetStdHandle(STD_INPUT_HANDLE);
  std::array<bool, 256> keys_{};
  std::array<bool, 256> pressed_{};
  bool key(unsigned k) const { return k < keys_.size() && keys_[k]; }
  bool press(unsigned k) const { return k < pressed_.size() && pressed_[k]; }

public:
  bool focused = true;
  explicit TuiInput(InputMode mode) {
    if (mode == InputMode::Ssh || mode == InputMode::SshBasic)
      throw std::runtime_error(
          "On Windows, run terminal_velocity.exe directly. For SSH, run the "
          "Linux build on the remote host.");
  }
  bool ready() const {
    DWORD mode = 0;
    return GetConsoleMode(input_, &mode);
  }
  void clear() {
    keys_.fill(false);
    pressed_.fill(false);
  }
  bool basic_ssh() const { return false; }
  std::string label() const { return "Windows console • press/release"; }
  // Public to allow platform input tests without synthesizing physical keys.
  void feed(const INPUT_RECORD &event) {
    if (event.EventType == FOCUS_EVENT) {
      focused = event.Event.FocusEvent.bSetFocus != FALSE;
      if (!focused)
        clear();
    } else if (event.EventType == KEY_EVENT && focused) {
      unsigned k = event.Event.KeyEvent.wVirtualKeyCode;
      if (k >= keys_.size())
        return;
      bool down = event.Event.KeyEvent.bKeyDown != FALSE;
      if (down && !keys_[k])
        pressed_[k] = true;
      keys_[k] = down;
    }
  }
  Controls consume() {
    Controls c;
    c.move = int(key('D') || key(VK_RIGHT)) - int(key('A') || key(VK_LEFT));
    c.jump_held = key(VK_SPACE) || key('W') || key(VK_UP);
    c.jump_pressed = press(VK_SPACE) || press('W') || press(VK_UP);
    c.dash_pressed = press(VK_SHIFT) || press(VK_LSHIFT) || press(VK_RSHIFT) ||
                     press('X') || press('K');
    c.fast_fall = key('S') || key(VK_DOWN);
    c.fast_fall_pressed = press('S') || press(VK_DOWN);
    c.aim_y = int(c.fast_fall) - int(key('W') || key(VK_UP));
    c.restart = press('R');
    c.new_run = press('N') || (press('R') && key(VK_CONTROL));
    c.quit = press('Q') || (press('C') && key(VK_CONTROL));
    c.select_level = press('1')   ? 1
                     : press('2') ? 2
                     : press('3') ? 3
                     : press('4') ? 4
                     : press('5') ? 5
                     : press('6') ? 6
                     : press('7') ? 7
                                  : 0;
    c.mute_audio = press('U');
    c.interact = press('E') || press('F');
    c.map = press('J');
    c.language = press('L');
    c.tour_page = int(press(VK_NEXT)) - int(press(VK_PRIOR));
    c.navigate = int(press('S') || press(VK_DOWN)) - int(press('W') || press(VK_UP));
    c.browse_leg = int(press('D') || press(VK_RIGHT)) - int(press('A') || press(VK_LEFT));
    c.next_seed = press('G');
    c.menu_request = press('M');
    c.pause = press(VK_ESCAPE) || press('P');
    c.confirm = press(VK_RETURN);
    c.journal = press(VK_TAB);
    c.debug = press(VK_F3);
    pressed_.fill(false);
    return c;
  }
  Controls poll(double) {
    // Query before reading: ReadConsoleInput otherwise blocks the physics loop.
    for (int batch = 0; batch < 4; ++batch) {
      DWORD available = 0, count = 0;
      if (!GetNumberOfConsoleInputEvents(input_, &available) || !available)
        break;
      INPUT_RECORD events[128];
      if (!ReadConsoleInputW(input_, events, std::min<DWORD>(available, 128),
                             &count))
        break;
      for (DWORD i = 0; i < count; ++i)
        feed(events[i]);
    }
    return consume();
  }
};
