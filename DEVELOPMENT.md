# Extending the iceberg

The executable uses one `AdventureSession`, one 120 Hz simulation and one Unicode cell framebuffer. The terminal writes ANSI; the window draws those same cells with SDL/FreeType. The original graphical SDL renderer remains a diagnostic option.

## Content and runtime boundaries

| File | Responsibility |
| --- | --- |
| `iceberg_types.hpp` | Stable types/IDs, district registry, profile, presentation view |
| `iceberg_world.hpp`, `iceberg_rooms_heat.inc` | Traversal templates, compatible ports, rhythm composition, room assembly |
| `iceberg_sequences.hpp`, `iceberg_missions.hpp`, `iceberg_gardens.hpp`, `iceberg_mastery.hpp` | Fixed lessons, story sequences and movement examination |
| `iceberg_story.hpp` | Bilingual dialogue, character portraits, task definitions and reward writing |
| `iceberg_runtime.hpp` | Campaign progression, interactions, revisits, expeditions, records and shared events |
| `iceberg_save.hpp` | Checksummed versioned saves, migration, verified atomic replacement and recovery |
| `iceberg_render.hpp`, `iceberg_scenery.hpp` | Unicode scenery, portraits, menus, journal, map and HUD |
| `iceberg_validation.hpp` | Bounded real-physics route validation and authored fallback |
| `main.cpp` | Existing input backends, shared physics, terminal sink and legacy modes |
| `sdl_frontend.cpp` | Window input, fonts and cell sink; retained graphical diagnostic renderer |

Six districts, the beacon ending, and a playable harbor epilogue are authored. `AuthoredDistrictCount` is the availability boundary; it is separate from the six-district world registry. Any future area must add verified geometry, writing and presentation. Raising the boundary must follow authoring and verification, not substitute palette swaps for areas.

Adding a district is content work: provide its twelve distinct patterns, settlement, fixed sequences, area-specific mechanics, scenery, conversations and tasks; validate ordinary movement and then raise the boundary. A profile at the previous boundary advances on continue. The existing profile already carries the remaining district/task/record slots.

## Stable identities and progression

Traversal IDs are 0–71, with twelve per district. Existing fixed lessons/finales use 72–83; harbor missions use 300–302; the shared mastery examination uses 320. Reserve new IDs without recycling published identities. Optional discoveries have separate stable IDs and never participate in mandatory completion checks.

Campaign stages are arrival, lesson, route, return, route, return, route, finale. An itinerary stores all three eight-room legs. Fixed interludes interrupt a leg's ending and save their own active state. Quest locations are placed before route validation. Return visits to completed fixed scenes preserve the campaign checkpoint separately.

`ContentVersion` belongs to geometry/route identity. Bump it when a published route's physics or geometry changes; new areas alone need not invalidate old rooms. Live ghosts match version, district, mode, template and seed. Records for explicit seeds also carry content version. Incompatible ghosts must never appear together.

The current writer is TVICE5; TVICE2/3 fixtures exercise migration. Preserve legacy TVP1 records in their own file. Any new serialized state requires an explicit format migration and interrupted-write tests. Checkpoint saves must retain moving-platform time, machinery, collected items and itinerary, rather than regenerate a new route on death.

## Verification

`make test` is the regular Linux suite. Native Windows CI uses `make windows windows-tests`; Linux-only process-kill tests are not run on Windows. No additional CI workflow permissions are needed for tests added to these existing targets.

- `iceberg_tests 10000 1000`: deterministic structure and 1,000 distributed ordinary-input routes, plus state/migration/identity regressions.
- `iceberg_tests 10000 1000 expert`: the expert variants; report stable and generic fallback counts separately.
- `iceberg_branch_tests 2`: every optional ledge and return path, all templates, two difficulty settings and two seeds.
- `iceberg_campaign_tests`: continuous critical path through the currently available story.
- `iceberg_story_playtest`: continuous journey including all available character tasks; reading time is explicitly estimated.
- `iceberg_quest_tests`: task ordering, rewards, revisits and optional discovery progression.
- `iceberg_mastery_tests`: actual eight-room trials and all three advanced movement techniques.
- `iceberg_text_tests`: actual 76×24 cell-renderer coverage of Polish and English dialogue.
- `iceberg_save_crash_tests`: 200 child-process interruptions during save replacement.
- `iceberg_metrics`: structural diversity, branching, ascent/reversals, rest points and difficulty distribution. Seed count alone is not a diversity metric.
- `tools/test_iceberg_tui.py`: actual terminal flow, language, resize, save/continue and terminal restoration.

`iceberg_search.hpp` is test-only: it searches ordinary control sequences and replays them through the same simulation/session. It never bypasses collisions or teleports during a route. Tests that install a room fixture are distinct from the continuous story playthrough.

Generation tries normal variation, then stable timing, then a verified recovery room. Preserve the hard bound. A high fallback rate is a content failure even if every generated route technically finishes. Review representative and worst-case rooms visually, in addition to numerical checks.

## Builds and assets

`tools/build_windows_sdl.sh` pins official SDL and FreeType archives by SHA-256. Set `TV_WINDOWS_BUILD_DIR` to reuse an external dependency directory. The portable window edition needs both executables, `SDL2.dll`, launchers, `assets/`, and `THIRD_PARTY.md`.

Audio source URLs, source/output hashes, processing and licenses live alongside the recordings. Font and rendering-library notices must accompany packages. Keep debug scene screenshots and simulations from writing real player profiles.

For release packaging, first build and verify the binaries, commit the reviewed tree, then run `python tools/package_release.py DESTINATION --windows`. It creates Linux and Windows archives, an archive of the exact committed source, per-bundle build identity files and SHA-256 checksums. It refuses a dirty working tree and does not touch player profiles or install anything. Run smoke tests from extracted archives before installing a release.

### Additive route revision (content version 3)

Layouts 0–23 retain their released geometry. New harbor layouts use IDs 400–405, heating works 420–425, with explicit district ownership through `district_of`. Never derive a new room's district from its ID. TVICE5 stores campaign and expedition `layout_version` fields; TVICE2/3/4 migration initializes them from the source content version. Existing itineraries, later endless bags and resumed trial records retain the old route version. New stories and expeditions use the current library. Add valid stable IDs to save validation when extending the registry. Preserve old geometry and old itinerary assembly for versioned runs.

### Browser frontend

`EMXX=/path/to/em++ ./tools/build_web.sh` produces `web/dist/`. Serve that folder
with an ordinary static HTTP(S) server. It contains the same C++ session,
120 Hz simulation, room validation, Unicode cell frames and recorded audio.
The web shell supplies keyboard/touch input, canvas glyph drawing and browser
save storage. Dialogue, inspection, menus and journal remain in the shared
cell renderer; narrow screens use its compact 38-column layout. There are no
HTML dialogue panels. Startup requires a click/key gesture for audio.

The browser edition stores its separate profile in IndexedDB; download a copy
using the save button. Desktop saves are not read or modified. Local storage
may be unavailable in private/restricted browsing. Classic legacy courses and
UDP ghosts remain desktop-only. An actual phone multitouch/play review is
still needed in addition to responsive viewport testing. Attribution and font
licenses are copied into the static distribution.

### Cultivation chambers

District 4 adds layouts 48–59, settlement 104 and sequences 80–81. Irrigation
sets persistent switch flags and unfolds `GrowingBridge` collision over 1.15
seconds. Rendering uses the same integer leading edge. Pause/focus freezes
both together; restoring a saved activation restores the complete bridge.
Celina's seeds use existing task 5 IDs and unlock outfit 5 (green), plus the
harbor planter. Notes 20–21 are ordinary instructions and never change
optional-discovery flags. The upper Crown is still unavailable content.

Web fullscreen targets `#playfield`; it keeps touch controls and cell-based
account overlays inside the surface. Restricted embedded/mobile browsers
fall back to an expanded game viewport with an exit control and Escape.
