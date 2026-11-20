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
    key(VK_DOWN, true);
    key(VK_DOWN, false);
    c = input.consume();
    check(c.fast_fall_pressed && !c.fast_fall, "short Down tap survives the native release");
    key(VK_LEFT, true);
    key(VK_DOWN, true);
    key('X', true);
    key('X', false);
    key(VK_DOWN, false);
    key(VK_LEFT, false);
    c = input.consume();
    check(c.move==0&&c.aim_y==0&&c.move_pressed==-1&&c.aim_pressed==1&&c.dash_pressed,
          "released Windows console directions keep the diagonal aim");
    EngineGrid open(80,60);
    open.spawn_x=20;open.spawn_y=24;
    auto dash=new_state(open);
    dash.player.facing=1;
    FixedStepper step;
    step.advance(dash,open,c,.001);
    c=input.consume();
    check(!c.move_pressed&&!c.aim_pressed,"Windows console direction edges are consumed once");
    step.advance(dash,open,c,.009);
    check(dash.player.vx<-20&&dash.player.vy>20,"the released keys launch a downward left dash");
    input.clear();
    key(VK_UP,true);key('X',true);key(VK_UP,false);key('X',false);key('D',true);
    c=input.consume();
    open.adventure_level=true;
    dash=new_state(open);step={};
    step.advance(dash,open,c,1./120);
    check(c.move==1&&std::abs(dash.player.vx)<1e-8&&dash.player.vy<-20,
          "Windows console preserves aim across events in the same read");
    input.clear();
    key(VK_LEFT, true);
    key('D', true);
    key(VK_UP, true);
    key('S', true);
    c=input.consume();
    check(c.move==0&&c.aim_y==0&&c.move_pressed==0&&c.aim_pressed==0,
          "opposing direction aliases cancel in the Windows console");
    input.clear();
    key(VK_NEXT, true);
    key(VK_NEXT, false);
    check(input.consume().tour_page==1,"native Page Down reaches Tour navigation");
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
    key('X',true);
    key(VK_SPACE,true);
    focus.Event.FocusEvent.bSetFocus=FALSE;
    input.feed(focus);
    key('A',true);
    focus.Event.FocusEvent.bSetFocus=TRUE;
    input.feed(focus);
    key('D',true);
    c=input.consume();
    check(input.focused&&c.focus_lost&&!c.dash_pressed&&!c.jump_pressed&&c.move==1&&
          !input.consume().focus_lost,"a native console focus bounce preserves the loss once and keeps fresh movement");
    input.clear();
    key('5', true);
    key('U', true);
    c = input.consume();
    check(c.select_level == 5 && c.mute_audio,
          "native campaign selection and audio mute controls");
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
      bool guard_ready = false, native_poll = false, native_dash = false;
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
        native.clear();
        const WORD code[]={VK_LEFT,VK_DOWN,'X',VK_LEFT,VK_DOWN,'X',VK_RIGHT};
        INPUT_RECORD taps[std::size(code)]{};
        for(size_t i=0;i<std::size(taps);++i) {
          taps[i].EventType=KEY_EVENT;
          taps[i].Event.KeyEvent.wVirtualKeyCode=code[i];
          taps[i].Event.KeyEvent.bKeyDown=i<3||i==6;
        }
        const bool queued=WriteConsoleInputW(console_input,taps,std::size(taps),&written)!=FALSE;
        from_console=native.poll(.01);
        native_dash=queued&&written==std::size(taps)&&from_console.dash_pressed&&
          from_console.move==1&&from_console.aim_y==0&&
          from_console.dash_pressed_direction==std::pair{-1,1};
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
      check(native_dash, "WriteConsoleInput preserves quick diagonal dash aim");
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
