CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pedantic
SDL_FLAGS = $(shell pkg-config --cflags --libs sdl2 freetype2)
ICEBERG_HEADERS = $(wildcard iceberg_*.hpp iceberg_*.inc)
.PHONY: all run test smoke clean
all: terminal_velocity keyboard_launcher terminal_velocity_sdl
terminal_velocity_sdl: sdl_frontend.cpp main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio.o
	$(CXX) $(CXXFLAGS) -o $@ sdl_frontend.cpp audio.o $(SDL_FLAGS) -ldl -lpthread
terminal_velocity: main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp audio.o
	$(CXX) $(CXXFLAGS) -o $@ main.cpp audio.o -ldl -lpthread
terminal_velocity terminal_velocity_sdl terminal_velocity.exe physics_tests extension_tests multiplayer_tests campaign_tests: $(ICEBERG_HEADERS)
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
iceberg_tests: iceberg_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_tests.cpp audio.o -ldl -lpthread
test: iceberg_compatibility_tests iceberg_harbor_tests sdl_cell_tests iceberg_route_tests iceberg_ui_tests physics_tests extension_tests multiplayer_tests campaign_tests iceberg_tests iceberg_campaign_tests iceberg_quest_tests iceberg_text_tests iceberg_story_playtest iceberg_mastery_tests iceberg_branch_tests iceberg_save_crash_tests iceberg_jubilee_tests iceberg_inspection_tests iceberg_reaction_tests terminal_velocity_sdl iceberg_dialogue_tests iceberg_ending_tests
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./sdl_cell_tests
	./iceberg_compatibility_tests
	./iceberg_harbor_tests
	./iceberg_route_tests
	./iceberg_ui_tests
	./physics_tests
	./extension_tests
	./multiplayer_tests
	./campaign_tests
	./iceberg_tests 10000 1000
	./iceberg_campaign_tests
	./iceberg_quest_tests
	./iceberg_text_tests
	./iceberg_story_playtest
	./iceberg_mastery_tests
	./iceberg_branch_tests
	./iceberg_save_crash_tests
	./iceberg_jubilee_tests
	./iceberg_inspection_tests
	./iceberg_reaction_tests
	./iceberg_dialogue_tests
	./iceberg_ending_tests
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./terminal_velocity_sdl --smoke
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./terminal_velocity_sdl --terminal-view --smoke
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
windows-tests: iceberg_route_tests.exe iceberg_ui_tests.exe iceberg_harbor_tests.exe physics_tests.exe extension_tests.exe multiplayer_tests.exe windows_input_tests.exe campaign_tests.exe iceberg_tests.exe iceberg_campaign_tests.exe iceberg_text_tests.exe iceberg_story_playtest.exe iceberg_mastery_tests.exe iceberg_jubilee_tests.exe iceberg_inspection_tests.exe iceberg_reaction_tests.exe iceberg_dialogue_tests.exe iceberg_ending_tests.exe
	@if [ -n "$$MSYSTEM" ]; then ./iceberg_route_tests.exe && ./iceberg_ui_tests.exe && ./iceberg_harbor_tests.exe && ./iceberg_tests.exe 1000 100 && ./iceberg_campaign_tests.exe && ./iceberg_text_tests.exe && ./iceberg_story_playtest.exe && ./iceberg_mastery_tests.exe && ./iceberg_jubilee_tests.exe && ./iceberg_inspection_tests.exe && ./iceberg_reaction_tests.exe && ./iceberg_dialogue_tests.exe && ./iceberg_ending_tests.exe; fi
iceberg_story_playtest.exe: iceberg_story_playtest.cpp main.cpp $(ICEBERG_HEADERS) audio_windows.o
	$(MINGW_CXX) $(WINDOWS_FLAGS) -o $@ $< audio_windows.o -lws2_32
%_tests.exe: %_tests.cpp main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp $(ICEBERG_HEADERS) audio_windows.o
	$(MINGW_CXX) $(WINDOWS_FLAGS) -o $@ $< audio_windows.o -lws2_32

windows_icon.o: windows_icon.rc assets/terminal-velocity.ico
	$(MINGW_WINDRES) -i windows_icon.rc -o $@

audio.o: audio.cpp audio.hpp vendor/miniaudio.h
	$(CXX) $(CXXFLAGS) -c audio.cpp -o $@
audio_windows.o: audio.cpp audio.hpp vendor/miniaudio.h
	$(MINGW_CXX) $(WINDOWS_FLAGS) -c audio.cpp -o $@

iceberg_campaign_tests: iceberg_campaign_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_campaign_tests.cpp audio.o -ldl -lpthread

# Native MSYS2 SDL fallback. Linux cross builds can keep the standalone TUI
# target; native Windows builds require UCRT64 SDL/FreeType packages.
terminal_velocity_sdl.exe: sdl_frontend.cpp main.cpp ssh_input.hpp ghost_net.hpp net_platform.hpp windows_console.hpp progress.hpp audio.hpp $(ICEBERG_HEADERS) audio_windows.o windows_icon.o
	$(MINGW_CXX) $(CXXFLAGS) -static-libgcc -static-libstdc++ -o $@ sdl_frontend.cpp audio_windows.o windows_icon.o $(SDL_FLAGS) -lws2_32

iceberg_access_tests: iceberg_access_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_access_tests.cpp audio.o -ldl -lpthread

iceberg_quest_tests: iceberg_quest_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_quest_tests.cpp audio.o -ldl -lpthread

iceberg_metrics: iceberg_metrics.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_metrics.cpp audio.o -ldl -lpthread

iceberg_save_crash_tests: iceberg_save_crash_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_save_crash_tests.cpp audio.o -ldl -lpthread

iceberg_story_playtest: iceberg_story_playtest.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_story_playtest.cpp audio.o -ldl -lpthread

iceberg_text_tests: iceberg_text_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_text_tests.cpp audio.o -ldl -lpthread

iceberg_mastery_tests: iceberg_mastery_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_mastery_tests.cpp audio.o -ldl -lpthread

iceberg_branch_tests: iceberg_branch_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_branch_tests.cpp audio.o -ldl -lpthread

iceberg_jubilee_tests: iceberg_jubilee_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_jubilee_tests.cpp audio.o -ldl -lpthread

iceberg_inspection_tests: iceberg_inspection_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_inspection_tests.cpp audio.o -ldl -lpthread

iceberg_reaction_tests: iceberg_reaction_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_reaction_tests.cpp audio.o -ldl -lpthread

iceberg_dialogue_tests: iceberg_dialogue_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ iceberg_dialogue_tests.cpp audio.o -ldl -lpthread

# Offline, scripted-input gameplay footage; never reads or writes player saves.
trailer_capture: tools/capture_trailer.cpp sdl_frontend.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ tools/capture_trailer.cpp audio.o $(SDL_FLAGS) -ldl -lpthread

iceberg_ending_tests: iceberg_ending_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ $< audio.o -ldl -lpthread

iceberg_route_tests: iceberg_route_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ $< audio.o -ldl -lpthread

iceberg_ui_tests: iceberg_ui_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ $< audio.o -ldl -lpthread

sdl_cell_tests: sdl_cell_tests.cpp sdl_frontend.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ $< audio.o $(SDL_FLAGS) -ldl -lpthread

iceberg_harbor_tests: iceberg_harbor_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ $< audio.o -ldl -lpthread

iceberg_compatibility_tests: iceberg_compatibility_tests.cpp main.cpp $(ICEBERG_HEADERS) audio.o
	$(CXX) $(CXXFLAGS) -o $@ $< audio.o -ldl -lpthread
