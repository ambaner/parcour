# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

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
