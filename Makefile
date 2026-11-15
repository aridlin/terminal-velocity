CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pedantic
SDL_FLAGS = $(shell pkg-config --cflags --libs sdl2 freetype2)
.PHONY: all run test smoke clean
all: terminal_velocity keyboard_launcher terminal_velocity_sdl
terminal_velocity_sdl: sdl_frontend.cpp main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio.o
	$(CXX) $(CXXFLAGS) -o $@ sdl_frontend.cpp audio.o $(SDL_FLAGS) -ldl -lpthread
terminal_velocity: main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio.o
	$(CXX) $(CXXFLAGS) -o $@ main.cpp audio.o -ldl -lpthread
keyboard_launcher: keyboard_launcher.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
physics_tests: physics_tests.cpp main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio.o
	$(CXX) $(CXXFLAGS) -o $@ physics_tests.cpp audio.o -ldl -lpthread
extension_tests: extension_tests.cpp main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio.o
	$(CXX) $(CXXFLAGS) -o $@ extension_tests.cpp audio.o -ldl -lpthread
multiplayer_tests: multiplayer_tests.cpp main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio.o
	$(CXX) $(CXXFLAGS) -o $@ multiplayer_tests.cpp audio.o -ldl -lpthread
campaign_tests: campaign_tests.cpp main.cpp progress.hpp audio.hpp audio.o
	$(CXX) $(CXXFLAGS) -o $@ campaign_tests.cpp audio.o -ldl -lpthread
test: physics_tests extension_tests multiplayer_tests campaign_tests terminal_velocity_sdl
	./physics_tests
	./extension_tests
	./multiplayer_tests
	./campaign_tests
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./terminal_velocity_sdl --smoke
smoke: test
run: terminal_velocity keyboard_launcher
	./play.sh
clean:
	rm -f terminal_velocity terminal_velocity_sdl physics_tests extension_tests multiplayer_tests campaign_tests keyboard_launcher *.exe windows_icon.o audio.o audio_windows.o

# Native Windows console build (MSYS2 UCRT64), or cross-compile on Linux.
MINGW_CXX ?= x86_64-w64-mingw32-g++
MINGW_WINDRES ?= x86_64-w64-mingw32-windres
WINDOWS_FLAGS = -std=c++20 -O2 -Wall -Wextra -pedantic -static -static-libgcc -static-libstdc++
.PHONY: windows windows-tests
windows: terminal_velocity.exe
terminal_velocity.exe: main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio_windows.o windows_icon.o
	$(MINGW_CXX) $(WINDOWS_FLAGS) -o $@ main.cpp windows_icon.o audio_windows.o -lws2_32
windows-tests: physics_tests.exe extension_tests.exe multiplayer_tests.exe windows_input_tests.exe campaign_tests.exe
%_tests.exe: %_tests.cpp main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio_windows.o
	$(MINGW_CXX) $(WINDOWS_FLAGS) -o $@ $< audio_windows.o -lws2_32

windows_icon.o: windows_icon.rc assets/terminal-velocity.ico
	$(MINGW_WINDRES) -i windows_icon.rc -o $@

audio.o: audio.cpp audio.hpp vendor/miniaudio.h
	$(CXX) $(CXXFLAGS) -c audio.cpp -o $@
audio_windows.o: audio.cpp audio.hpp vendor/miniaudio.h
	$(MINGW_CXX) $(WINDOWS_FLAGS) -c audio.cpp -o $@
