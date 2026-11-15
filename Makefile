CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pedantic
SDL_FLAGS = $(shell pkg-config --cflags --libs sdl2 freetype2)
.PHONY: all run test smoke clean
all: terminal_velocity keyboard_launcher terminal_velocity_sdl
terminal_velocity_sdl: sdl_frontend.cpp main.cpp ssh_input.hpp
	$(CXX) $(CXXFLAGS) -o $@ sdl_frontend.cpp $(SDL_FLAGS)
terminal_velocity: main.cpp ssh_input.hpp
	$(CXX) $(CXXFLAGS) -o $@ main.cpp
keyboard_launcher: keyboard_launcher.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
physics_tests: physics_tests.cpp main.cpp ssh_input.hpp
	$(CXX) $(CXXFLAGS) -o $@ physics_tests.cpp
extension_tests: extension_tests.cpp main.cpp ssh_input.hpp
	$(CXX) $(CXXFLAGS) -o $@ extension_tests.cpp
test: physics_tests extension_tests terminal_velocity_sdl
	./physics_tests
	./extension_tests
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./terminal_velocity_sdl --smoke
smoke: test
run: terminal_velocity keyboard_launcher
	./play.sh
clean:
	rm -f terminal_velocity terminal_velocity_sdl physics_tests extension_tests keyboard_launcher
