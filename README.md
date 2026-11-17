# Terminal Velocity — Beneath the Surface

A courier platformer inside an inhabited iceberg. Carry the mail, reconnect the heating network, and meet the people keeping an impossible city running.

**The terminal is the game.** Portraits, scenery and animation use Unicode cells: blocks, Braille, line drawing and lettering. The window edition displays the **same cell framebuffer**, with bundled fonts, when a terminal is inconvenient.

[Polska instrukcja](README.pl.md) · [Classic courses](docs/LEGACY.md) · [Development](DEVELOPMENT.md)

## The playable opening

Explore the **Lower Harbor**, **Lazarus Heating Works** **Windowsill Estate** and the **Archive of Silence**: four settlements, 60 authored traversal templates, lessons, machinery, moving barges, a stranded conductor and a fragile delivery. Seven optional character tasks unlock letters, service and tram shortcuts, a rooftop garden, outfits and movement trials. The opening resolves the lower network's immediate emergency; the ascent through the upper city continues in a later content release.

The current journey reaches the original network plan in the frozen archive. Automated traversal with all seven tasks plus estimated reading is about 32 minutes; this is **not a measured first-player duration**. This release is not the planned two-hour campaign, and the number of seeds is not a promise of hundreds of distinct hours.

Afterwards, revisit settlements and authored routes, start endless expeditions, or attempt the four district mastery trials. Each trial combines seven expert rooms with a final movement examination. Time, fewest retries and collection records are independent. Explicit seeds have separate records; mastery unlocks a visible badge and outfit.

Polish is the default. **L** in the menu switches to English, including dialogue, objectives and controls. Observation and exploration are rewarded, but optional discoveries never gate the ordinary story.

## Start playing

Linux, inside a terminal:

```sh
./play.sh
./play.sh --ssh                  # terminal protocol input; no evdev helper
./play.sh --terminal-view        # identical Unicode graphics in an SDL window
./ghostty-play.sh                # open a new Ghostty window
```

Use at least **76×24 cells**; **110×34** is recommended. The window can be resized; **F11** toggles fullscreen. **Ctrl +/−** changes cell size and **Ctrl 0** resets it; high-density displays scale automatically. Losing focus pauses play. **Enter/P** resumes.

On Windows 10/11 x64, extract the whole package and run **play.cmd** in Windows Terminal, or **play-window.cmd** for the window edition. Keep `assets/` beside the executables. The window edition needs the included `SDL2.dll`; the terminal executable does not. No administrator access is needed on Windows.

The default Linux input backend reads real evdev key presses and releases. If access is unavailable, `play.sh` uses the existing keyboard helper: it asks for sudo locally, opens the keyboard, permanently drops privileges, and runs the game as you. It changes no groups or device permissions. `--ssh` and the window edition avoid that helper.

## Controls

| Key | Action |
| --- | --- |
| A/D or left/right | Move |
| Space, W or up | Jump; hold for height |
| X or Shift + direction | Dash |
| S/down | Fast fall; drop through a moving platform |
| E/F | Talk, operate machinery, collect a task object |
| B | Detailed illustrated inspection of a nearby object or character; B closes without using it |
| E/Enter | Continue dialogue/inspection text or leave a completed room |
| R | Fast checkpoint retry |
| Tab | Journal; select a completed task and E to read its reward |
| J | District map; W/S selects, Enter travels when available |
| P/Escape | Pause or close a panel |
| M | Main menu |
| U | Mute/unmute |
| Q | Save and quit |

The amber body means a dash is ready; blue means it is spent or temporarily restricted by a delivery. Pale cyan **✦** refills restore the dash. Warm **◆** treasures are optional. Lamps mark checkpoints. Bright platform edges are collidable; dim architecture is scenery. Red hazards remain dangerous during a dash.

Three techniques extend the original forgiving movement: **dash along the ground, then jump** to keep momentum; **dash upward beside a wall, then jump** to bounce away; **jump from a rising platform** to inherit its lift. Lessons have nearby checkpoints, and expert routes combine these techniques.

The menu offers continue/new story, endless expeditions, mastery trials, settings and classic courses. Use **J** on an expedition/trial entry to choose an unlocked district. **Enter** resumes its saved run; **R/N** starts a fresh one. A new story replaces story progress after confirmation, while keeping mastery records, outfits and separate expeditions.

## Seeds, retries and saves

```sh
./play.sh --expedition --seed 73
./play.sh --trial --seed 73 --district 2
./play.sh --campaign --language en
./play.sh --legacy --summit
```

Districts unlock through the journey. A trial unlocks after Kuba's movement task or the end of this opening. Without an explicit seed, a new expedition gets a new layout. With `--seed`, a fresh run repeats that challenge. For compatibility, **`--seed` alone selects the old procedural mode**; include `--campaign`, `--expedition` or `--trial` for the iceberg game.

Death and continue preserve the assembled route. Retry returns to the latest lamp; story and task progress remain. Saves contain campaign, expedition and trial checkpoints separately, with verified atomic replacement and a recovery copy.

- Linux: `$XDG_STATE_HOME/terminal-velocity/progress.dat.iceberg`, normally `~/.local/state/terminal-velocity/progress.dat.iceberg`.
- Windows: `%LOCALAPPDATA%/TerminalVelocity/progress.dat.iceberg`.
- `TV_ICEBERG_FILE` selects a separate iceberg profile. `TV_PROGRESS_FILE` changes the legacy profile path and its default `.iceberg` companion.

Old `progress.dat` records remain intact. Keep both files when backing up. Future save versions are protected from overwrite. Saves at the end of the opening can continue when more districts are authored.

## Sound and SSH

Recorded music, harbor water, heating machinery, material footsteps and character cues share the same engine in both frontends. Settings have separate music/effects/ambience levels and outfit selection. **1/2/3** cycles the volume groups; **4** cycles unlocked outfits.

SSH is quiet by default. `--audio` enables sound on the remote game host, not streaming to your client. `--mute` starts silently. Sources, modifications, licenses and hashes are in [audio credits](assets/audio/CREDITS.txt) and the [manifest](assets/audio/manifest.json).

`--ssh` negotiates kitty keyboard events for simultaneous presses/releases. When unavailable, **basic mode** uses A/D to start running, S to stop, Space to jump and X to dash. **I/O** dashes upward/downward; the current run direction makes a diagonal, so press S first for a vertical dash. Basic jumping uses a fixed hold interval. `--ssh-basic` forces this mode.

## Live ghosts

```sh
./terminal_velocity --relay 27888
./play.sh --expedition --seed 73 --connect HOST:27888 --room friends
```

Use the same game/content version, district, mode and seed. Ghosts appear only on compatible routes, have no collision, and disappear during menus, dialogue or focus loss. Use the updated relay for iceberg routes. Legacy racers remain separate. The relay uses UDP; room names are grouping labels, not passwords. There is no shared story state or recorded replay system.

## Build and verify

Linux needs a C++20 compiler and make. SDL2 and FreeType are needed only for the window. Debian/Ubuntu build packages: `g++ make pkg-config libsdl2-dev libfreetype6-dev`; Arch/CachyOS: `base-devel pkgconf sdl2-compat freetype2`.

```sh
make -j2 all
make test
python tools/test_iceberg_tui.py   # requires Python pyte; real PTY flows
./iceberg_tests 10000 1000 expert
./iceberg_branch_tests 2
```

`make terminal_velocity` builds the terminal without graphical libraries. `make test` covers legacy physics/networking, 10,000 structural seeds, 1,000 simulated routes, story/task progression, all template branches, mastery, compact bilingual dialogue, interrupted saves and SDL smoke checks. These are reproducible checks, not proof about every possible seed or human playtime.

Native Windows terminal build in MSYS2 UCRT64:

```sh
make windows windows-tests MINGW_CXX=g++ MINGW_WINDRES=windres
```

For the native window build, install UCRT64 SDL2/FreeType development packages and use `make terminal_velocity_sdl.exe MINGW_CXX=g++`. On Linux with MinGW-w64, **tools/build_windows_sdl.sh** downloads checksum-pinned official dependencies and builds both executables plus `SDL2.dll`. See [third-party notices](THIRD_PARTY.md).

GitHub CI tests Linux and native Windows terminal/shared logic. Its existing Windows artifact contains the terminal edition; the separately prepared **window edition** also includes the SDL executable and DLL. Windows window testing includes cross-compilation and Wine; physical Windows graphics/input testing remains separate.

The original graphical frontend is retained with `--diagnostic`. The event, mountain chapters and precision summit remain available through **Classic courses**; the [older guide](docs/LEGACY.md) documents those modes.
