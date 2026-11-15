#define TERMINAL_VELOCITY_NO_TUI_MAIN
#include "main.cpp"
#include <stdexcept>

void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
  std::cout << "PASS " << message << '\n';
}
int main() {
#ifdef _WIN32
  try {
    TuiInput input(InputMode::Auto);
    auto key = [&](WORD code, bool down) {
      INPUT_RECORD event{};
      event.EventType = KEY_EVENT;
      event.Event.KeyEvent.wVirtualKeyCode = code;
      event.Event.KeyEvent.bKeyDown = down;
      input.feed(event);
    };
    key('D', true);
    key(VK_SPACE, true);
    auto c = input.consume();
    check(c.move == 1 && c.jump_pressed && c.jump_held,
          "native simultaneous movement and jump");
    key(VK_SPACE, true);
    c = input.consume();
    check(!c.jump_pressed && c.jump_held, "autorepeat does not retrigger jump");
    key(VK_SPACE, false);
    key('D', false);
    c = input.consume();
    check(!c.jump_held && c.move == 0, "native releases clear movement");
    key('X', true);
    key('X', false);
    check(input.consume().dash_pressed,
          "short native tap survives release before tick");
    check(!input.consume().dash_pressed, "tap edge consumed only once");
    key('W', true);
    key('D', true);
    key(VK_SHIFT, true);
    c = input.consume();
    check(c.move == 1 && c.aim_y == -1 && c.dash_pressed,
          "native diagonal dash input");
    INPUT_RECORD focus{};
    focus.EventType = FOCUS_EVENT;
    focus.Event.FocusEvent.bSetFocus = FALSE;
    input.feed(focus);
    c = input.consume();
    check(!input.focused && c.move == 0 && !c.jump_held,
          "focus loss clears held keys");
    key('D', true);
    check(input.consume().move == 0, "unfocused key events ignored");
    focus.Event.FocusEvent.bSetFocus = TRUE;
    input.feed(focus);
    key('3', true);
    key(VK_RETURN, true);
    c = input.consume();
    check(input.focused && c.select_level == 3 && c.confirm,
          "native precision course menu controls");
    input.clear();
    key(VK_CONTROL, true);
    key('R', true);
    check(input.consume().new_run, "native Ctrl+R retries run");
    input.clear();
    key(VK_CONTROL, true);
    key('C', true);
    check(input.consume().quit, "native Ctrl+C clean quit");
    if (std::getenv("TV_TEST_CONSOLE")) {
      bool created = AllocConsole() != FALSE;
      HANDLE old_input = GetStdHandle(STD_INPUT_HANDLE),
             old_output = GetStdHandle(STD_OUTPUT_HANDLE);
      HANDLE console_input =
          CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                      OPEN_EXISTING, 0, nullptr);
      HANDLE console_output =
          CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                      OPEN_EXISTING, 0, nullptr);
      check(console_input != INVALID_HANDLE_VALUE &&
                console_output != INVALID_HANDLE_VALUE,
            "opens native console handles");
      SetStdHandle(STD_INPUT_HANDLE, console_input);
      SetStdHandle(STD_OUTPUT_HANDLE, console_output);
      DWORD before_input = 0, before_output = 0;
      GetConsoleMode(console_input, &before_input);
      GetConsoleMode(console_output, &before_output);
      UINT before_cp = GetConsoleCP(), before_output_cp = GetConsoleOutputCP();
      std::ostringstream captured;
      auto old_buffer = std::cout.rdbuf(captured.rdbuf());
      bool guard_ready = false, native_poll = false;
      {
        TerminalGuard terminal;
        guard_ready = terminal.active() && GetConsoleOutputCP() == CP_UTF8;
        FlushConsoleInputBuffer(console_input);
        INPUT_RECORD events[3]{};
        for (auto &e : events) {
          e.EventType = KEY_EVENT;
          e.Event.KeyEvent.bKeyDown = TRUE;
        }
        events[0].Event.KeyEvent.wVirtualKeyCode = 'D';
        events[1].Event.KeyEvent.wVirtualKeyCode = VK_SPACE;
        events[2].Event.KeyEvent.wVirtualKeyCode = VK_SPACE;
        events[2].Event.KeyEvent.bKeyDown = FALSE;
        DWORD written = 0;
        WriteConsoleInputW(console_input, events, 3, &written);
        TuiInput native(InputMode::Auto);
        auto from_console = native.poll(.01);
        native_poll = written == 3 && from_console.move == 1 &&
                      from_console.jump_pressed && !from_console.jump_held;
      }
      std::cout.rdbuf(old_buffer);
      DWORD after_input = 0, after_output = 0;
      GetConsoleMode(console_input, &after_input);
      GetConsoleMode(console_output, &after_output);
      bool restored = before_input == after_input &&
                      before_output == after_output &&
                      GetConsoleCP() == before_cp &&
                      GetConsoleOutputCP() == before_output_cp;
      SetStdHandle(STD_INPUT_HANDLE, old_input);
      SetStdHandle(STD_OUTPUT_HANDLE, old_output);
      CloseHandle(console_input);
      CloseHandle(console_output);
      if (created)
        FreeConsole();
      check(guard_ready, "native console enables VT output and UTF-8");
      check(native_poll,
            "ReadConsoleInput preserves simultaneous keys and short taps");
      check(restored, "native console modes and code pages restored");
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
#else
  std::cout << "Windows input tests require a Windows build\n";
  return 0;
#endif
}
