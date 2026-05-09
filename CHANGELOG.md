# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [1.1.0] — 2026-05-08

### Added
- **Level editor** — standalone Win32 GUI (`editor.exe`) for creating `.parcour` level files with tile painting, clearance warnings, and enclosure validation
- **`.parcour` file format** — binary level file format with header, 40×30 tile grid, spawn validation
- **Random level generator** — `levelgen.c` produces random maze levels with guaranteed connectivity and enclosure
- **CLI level loading** — `parcour.exe mymap.parcour` loads custom levels from the command line
- **Tiered barrier interaction** — foot-level barriers auto-step-up (20-frame cooldown), chest-level barriers block (manual jump), head-level barriers trigger pull-up
- **Overlap-aware horizontal collision** — full-body tile_solid checks at 5 heights with overlap filter to prevent getting stuck under ceiling platforms
- **Version info resources** — all binaries (`parcour.exe`, `test_runner.exe`, `editor.exe`) embed Win32 VERSIONINFO with product name, version 1.1.0.0, copyright, and file descriptions
- **Shared version header** — `src/version.h` centralizes version constants across all binaries
- **29 new tests** (52 → 81) — levelfile (7), levelgen (9), input (4), renderer (3), sprite (3), animation (7), plus expanded physics, regression, and sweep tests
- **Test levels** — test3 (sealed room), test4 (low ceiling), test5 (single-tile walls), test6 (staircase)

### Changed
- **Horizontal collision rewrite** — now uses `tile_solid()` at all 5 body heights (not `tile_wall()` at mid+low+foot); leading-edge-only with overlap-aware filtering
- **Character struct** — added `stepCooldown` field for auto-step-up rate limiting
- **Build pipeline** — now 5-stage: engine.lib → sprites.res → version resources → parcour.exe/test_runner.exe/editor.exe
- **Design document** — comprehensive update to v1.1 (~1550 lines), rewrote physics sections, added barrier tiers, level system, collision journey iteration #6

### Fixed
- **Character stuck under ceiling** — after corner-climbing to a position under ceiling platforms, horizontal movement was blocked in both directions; overlap-aware filter now allows movement when already inside a solid tile
- **4 test regressions** from collision rewrite — repositioned test scenarios to avoid false reliance on floor/OOB tile blocking

---

## [1.0.0] — 2026-04-27

### Added
- **Single-file deployment** — all 44 sprite PNGs embedded as Win32 RCDATA resources; game ships as one ~255 KB executable
- **Static CRT linking** (`/MT` / `/MTd`) — zero runtime dependencies on any 64-bit Windows
- **Engine/Game architecture** — engine builds as `engine.lib` (static library), game links against it
- **15-state character FSM** — idle, run, jump, fall, crouch, land, wall-slide, corner-grab, corner-climb, somersault, slide, hard-landing, edge-stop, turn, and stand
- **PoP-style physics** — quadratic gravity with air drag, asymptotic terminal velocity
- **Tile-based collision** — AABB probes against a 40×30 grid with wall vs. platform distinction
- **Ledge mechanics** — corner-grab from air, head-height grab from ground, auto-climb on forward hold
- **Framebuffer renderer** — direct `UINT32[]` pixel manipulation, `StretchDIBits` to `HWND`
- **Dual-mode sprite loading** — embedded resources (primary) with disk fallback for development
- **52 automated tests** across 10 categories (math, gravity, physics, character, level, input, renderer, sprite, animation, integration)
- **Custom test framework** — `test_framework.h` with `TEST()`, `ASSERT_TRUE()`, `ASSERT_EQ()`, `RUN_SUITE()` macros
- **Logging system** — file + `OutputDebugString` dual-output, per-run log file
- **4-stage build pipeline** — `build.bat` automates: engine.lib → sprites.res → parcour.exe → test_runner.exe

### Project Structure
- `src/engine/` — reusable 2D engine (math, gravity, physics, input, renderer, log)
- `src/game/` — game-specific code (character, level, sprite, game loop)
- `test/engine/` — engine unit tests
- `test/game/` — game unit tests and integration tests
- `doc/DESIGN.md` — comprehensive design document (~1400 lines)

---

## [0.3.0] — 2026-04-26

### Added
- Engine extracted as static library (`engine.lib`)
- Test files split by module into `test/engine/` and `test/game/` subfolders
- Test helpers (`make_test_player()`, `make_test_level()`)
- Bug regression tests and deterministic replay tests

### Changed
- Source reorganized into `src/engine/` and `src/game/` subdirectories

---

## [0.2.0] — 2026-04-26

### Added
- Wall vs. platform collision distinction (thin platforms passable horizontally)
- Head-bonk collision (ceiling detection)
- Window resize handling with aspect ratio preservation
- Logging system (`log.c` / `log.h`)
- Comprehensive test suite (52 tests)
- Custom test framework (`test_framework.h`)

### Fixed
- Fall-through-floor bug at high velocities
- Edge detection inconsistency at tile boundaries

---

## [0.1.0] — 2026-04-26

### Added
- Initial project: game loop, WndProc, framebuffer renderer
- Basic gravity and physics
- Character sprite loading from PNG files (stb_image)
- 14-frame PoP-accurate animation system
- Tile-based level with solid/empty tiles
- Keyboard input handling (arrow keys)
