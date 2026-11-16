# Extending the iceberg

The executable uses one `AdventureSession`, one 120 Hz simulation and one Unicode cell framebuffer. The terminal writes ANSI; the window draws those same cells with SDL/FreeType. The original graphical SDL renderer remains a diagnostic option.

## Content and runtime boundaries

| File | Responsibility |
| --- | --- |
| `iceberg_types.hpp` | Stable types/IDs, district registry, profile, presentation view |
| `iceberg_world.hpp`, `iceberg_rooms_heat.inc` | Traversal templates, compatible ports, rhythm composition, room assembly |
| `iceberg_sequences.hpp`, `iceberg_missions.hpp`, `iceberg_mastery.hpp` | Fixed lessons, story sequences and movement examination |
| `iceberg_story.hpp` | Bilingual dialogue, character portraits, task definitions and reward writing |
| `iceberg_runtime.hpp` | Campaign progression, interactions, revisits, expeditions, records and shared events |
| `iceberg_save.hpp` | Checksummed versioned saves, migration, verified atomic replacement and recovery |
| `iceberg_render.hpp`, `iceberg_scenery.hpp` | Unicode scenery, portraits, menus, journal, map and HUD |
| `iceberg_validation.hpp` | Bounded real-physics route validation and authored fallback |
| `main.cpp` | Existing input backends, shared physics, terminal sink and legacy modes |
| `sdl_frontend.cpp` | Window input, fonts and cell sink; retained graphical diagnostic renderer |

Two districts are fully authored. `AuthoredDistrictCount` is the availability boundary; it is separate from the six-district world registry. Future district introductions, character identities and audio slots are scaffolding, not completed gameplay. Raising the boundary must follow authoring and verification, not substitute palette swaps for areas.

Adding a district is content work: provide its twelve distinct patterns, settlement, fixed sequences, area-specific mechanics, scenery, conversations and tasks; validate ordinary movement and then raise the boundary. A profile at the previous boundary advances on continue. The existing profile already carries the remaining district/task/record slots.

## Stable identities and progression

Traversal IDs are 0–71, with twelve per district. Existing fixed lessons/finales use 72–83; harbor missions use 300–302; the shared mastery examination uses 320. Reserve new IDs without recycling published identities. Optional discoveries have separate stable IDs and never participate in mandatory completion checks.

Campaign stages are arrival, lesson, route, return, route, return, route, finale. An itinerary stores all three eight-room legs. Fixed interludes interrupt a leg's ending and save their own active state. Quest locations are placed before route validation. Return visits to completed fixed scenes preserve the campaign checkpoint separately.

`ContentVersion` belongs to geometry/route identity. Bump it when a published route's physics or geometry changes; new areas alone need not invalidate old rooms. Live ghosts match version, district, mode, template and seed. Records for explicit seeds also carry content version. Incompatible ghosts must never appear together.

The current writer is TVICE4; TVICE2/3 fixtures exercise migration. Preserve legacy TVP1 records in their own file. Any new serialized state requires an explicit format migration and interrupted-write tests. Checkpoint saves must retain moving-platform time, machinery, collected items and itinerary, rather than regenerate a new route on death.

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
