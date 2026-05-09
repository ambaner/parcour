# Parcour — Design Document

> **Version:** 1.1  
> **Date:** April 2026  
> **Author:** ambaner  
> **Status:** Active Development  

---

## Table of Contents

0. [Getting Started](#0-getting-started)
1. [Project Overview](#1-project-overview)
2. [Goals and Non-Goals](#2-goals-and-non-goals)
3. [Architecture](#3-architecture)
4. [Source Code Organization](#4-source-code-organization)
5. [Module Reference](#5-module-reference)
6. [Data Structures](#6-data-structures)
7. [Game Loop and Frame Pipeline](#7-game-loop-and-frame-pipeline)
8. [Physics System](#8-physics-system)
9. [Character State Machine](#9-character-state-machine)
10. [Level System](#10-level-system)
11. [Sprite and Rendering Pipeline](#11-sprite-and-rendering-pipeline)
12. [Logging and Telemetry](#12-logging-and-telemetry)
13. [Build System](#13-build-system)
14. [Debugging Tips and Tricks](#14-debugging-tips-and-tricks)
15. [Key Design Decisions](#15-key-design-decisions)
16. [Known Limitations and Future Work](#16-known-limitations-and-future-work)
17. [Appendix A: Constants Reference](#appendix-a-constants-reference)
18. [Appendix B: Level Layout](#appendix-b-level-layout)
19. [Appendix C: Sprite Asset Inventory](#appendix-c-sprite-asset-inventory)

---

## 0. Getting Started

This section walks you through setting up and building the project from scratch on a fresh Windows machine.

### What You Need

| Requirement | Details |
|-------------|---------|
| **Operating System** | Windows 10 or later (64-bit) |
| **Visual Studio** | 2019 or later — any edition, including the free **Community** edition |
| **VS Workload** | **"Desktop development with C++"** must be selected during installation |

Visual Studio provides the entire toolchain: `cl.exe` (compiler), `link.exe` (linker), `lib.exe` (librarian), `rc.exe` (resource compiler), and the Windows SDK (headers + import libraries). No other tools or package managers are required.

> **Already have Visual Studio?** Make sure the "Desktop development with C++" workload is installed. Open the Visual Studio Installer → Modify → check the workload if it's not already selected.

### Installing Visual Studio (if you don't have it)

1. Download [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/) (free).
2. Run the installer.
3. In the **Workloads** tab, check **"Desktop development with C++"**.
4. Click **Install**. This installs `cl.exe`, the Windows SDK, and all build tools.
5. No need to open Visual Studio itself — we build from the command line.

### Clone and Build

```bash
# 1. Clone the repository
git clone https://github.com/YOUR_USERNAME/Parcour.git
cd Parcour

# 2. Open a Developer Command Prompt
#    (Start Menu → "Developer Command Prompt for VS 2022")
#    Or from PowerShell:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# 3. Build everything and run tests
build.bat fre

# 4. Run the game
build\amd64fre\parcour.exe
```

### What `build.bat fre` Does

The build script runs a 4-stage pipeline:

```
Stage 1:  cl.exe compiles engine .c files → engine.lib (static library)
Stage 2:  rc.exe compiles sprites.rc      → sprites.res (44 embedded PNGs)
Stage 3:  cl.exe links game + engine.lib + sprites.res → parcour.exe
Stage 4:  cl.exe links tests + engine.lib              → test_runner.exe (auto-runs)
```

On success you'll see:
```
  Results: 81 passed, 0 failed, 81 total
Done.
```

### Build Configurations

| Command | Config | Output | Use Case |
|---------|--------|--------|----------|
| `build.bat fre` | Release | `build/amd64fre/` | Normal use — optimized, static CRT |
| `build.bat chk` | Debug | `build/amd64chk/` | Debugging — symbols, no optimization |
| `build.bat all` | Both | Both dirs | Build + test both configs |
| `build.bat clean` | — | — | Delete all build artifacts |
| `build.bat notest` | Release | `build/amd64fre/` | Build without running tests |

### Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| `'cl' is not recognized` | MSVC tools not on PATH | Use a **Developer Command Prompt**, or run `vcvars64.bat` first |
| `fatal error C1034: stdio.h: no include path set` | `cl.exe` found but environment not set up | Same fix — run `vcvars64.bat` |
| `error LNK1168: cannot open parcour.exe` | Previous instance still running | Close the game window, or: `taskkill /IM parcour.exe /F` |
| Build succeeds but tests fail | Test assertion failed | Read the output — shows file, line, expected vs. actual |

### Controls

| Key | Action |
|-----|--------|
| **← →** | Walk / Run |
| **↑** | Jump |
| **↓** | Crouch |
| **Space** | Grab ledge |

---

## 1. Project Overview

**Parcour** is a learning project that recreates the core movement and physics mechanics of the 1989 Apple II game *Prince of Persia* (PoP) using modern tools. The goal is to understand — through hands-on implementation — how classic 2D platformers achieve fluid, realistic character movement, gravity, edge detection, wall interaction, and ledge climbing.

The project is written in **pure C** using the **Win32 API** with no game engine, no middleware, and no external dependencies beyond `stb_image.h` for PNG loading. All 44 sprite frames are embedded as Win32 resources (`RCDATA`), producing a **single self-contained executable** (~255 KB) that runs on any 64-bit Windows machine with zero runtime dependencies. The character is rendered using pre-rendered sprite sheets from the open-source "Animated Pixel Adventurer" asset pack by rvros (itch.io).

### What Makes This Interesting

Unlike tutorials that use game engines, this project builds every system from scratch:

- **Framebuffer rendering** — direct pixel manipulation via a `UINT32[]` array blitted to an `HWND` with `StretchDIBits`
- **Tile-based collision** — AABB probes against a 40×30 integer grid
- **State machine animation** — 15-state FSM driving sprite sheet playback
- **Realistic gravity** — quadratic air drag producing asymptotic terminal velocity
- **Full-body collision** — leading-edge-only horizontal checks at 5 body heights with overlap-aware filtering
- **Tiered barrier interaction** — auto-step-up for foot-level barriers, manual jump for chest-level, pull-up for head-level
- **PoP-style ledge mechanics** — corner-grab from air, head-height grab from ground, auto-climb on forward hold
- **Level editor** — standalone GUI editor for `.parcour` level files with clearance warnings and enclosure validation
- **Custom level loading** — command-line level file support (`parcour.exe mymap.parcour`)

---

## 2. Goals and Non-Goals

### Goals

| # | Goal | Status |
|---|------|--------|
| G1 | Understand and implement PoP-style character movement physics | ✅ Done |
| G2 | Implement realistic gravity with progressive acceleration and air drag | ✅ Done |
| G3 | Build a 15-state character FSM covering idle, walk, run, jump, fall, crouch, slide, wall-slide, corner-grab, corner-climb, somersault, hard-landing | ✅ Done |
| G4 | Implement full-body tile collision with leading-edge checks and overlap-aware filtering | ✅ Done |
| G5 | Support PoP-style ledge grabs (air corner-grab + grounded head-ledge grab + auto-climb) | ✅ Done |
| G6 | Provide diagnostic logging sufficient to trace and debug physics issues | ✅ Done |
| G7 | Keep the codebase modular, readable, and educational | ✅ Done |
| G8 | Level editor with `.parcour` file format, clearance warnings, and enclosure validation | ✅ Done |
| G9 | Tiered barrier interaction (auto-step-up, manual jump, pull-up) | ✅ Done |

### Non-Goals

- **Not a shipping game** — no menus, scoring, enemies, health, levels, or win conditions
- **Not cross-platform** — Win32 only, uses GDI for rendering
- **Not performance-optimized** — clarity over speed (e.g., per-pixel alpha blending in software)
- **Not a PoP clone** — inspired by its mechanics, not attempting to replicate its content

---

## 3. Architecture

### High-Level Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                          game.c (Entry Point)                       │
│  WinMain → Window Creation → Game Loop → Shutdown                   │
│                                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │
│  │ input.c  │→│character.c│→│ physics.c │→│   renderer.c      │   │
│  │ Keyboard │  │ State FSM │  │ Collision │  │   Framebuffer    │   │
│  │ Tracking │  │ Animation │  │ Gravity   │  │   Blitting       │   │
│  └──────────┘  └────┬─────┘  └─────┬─────┘  └────────┬─────────┘  │
│                     │              │                    │            │
│                ┌────┴──────┐  ┌───┴────┐          ┌───┴──────┐    │
│                │ sprite.c  │  │level.c │          │  log.c   │    │
│                │ PNG Load  │  │Tile Map│          │ File Log │    │
│                │ Alpha Blit│  │Queries │          │Timestamp │    │
│                └───────────┘  └────────┘          └──────────┘    │
│                                                                     │
│  ┌──────────┐  ┌──────────┐                                        │
│  │gravity.c │  │  math.c  │  Foundation layer (no dependencies)    │
│  │Air Drag  │  │ Utilities│                                        │
│  └──────────┘  └──────────┘                                        │
└─────────────────────────────────────────────────────────────────────┘
```

### Dependency Graph

```
types.h          ← root header (WINDOWS.H + constants)
  ├── math.h/c         ← pure math (no deps)
  ├── log.h/c          ← file logging (no deps beyond stdio + WinAPI)
  ├── gravity.h/c      ← gravity model (depends: math)
  ├── renderer.h/c     ← framebuffer + primitives (depends: types)
  ├── sprite.h/c       ← PNG loading + blitting (depends: renderer, stb_image)
  ├── input.h/c        ← keyboard state (depends: types)
  ├── level.h/c        ← tile map + queries (depends: renderer)
  ├── levelfile.h/c    ← .parcour file format parser/writer (depends: level, log)
  ├── levelgen.h/c     ← random maze generator (depends: level, log)
  ├── physics.h/c      ← collision resolution (depends: math, level)
  ├── character.h/c    ← state machine (depends: ALL above)
  └── game.c           ← entry point (depends: ALL above)
```

### Design Principles

1. **No engine, no framework** — every system is visible and debuggable
2. **Single-threaded** — the game loop is a classic poll-update-render cycle
3. **No heap allocation at runtime** — all memory is static arrays or stack-allocated (except sprite loading at startup)
4. **Modules communicate through function calls and shared constants** — no event bus, no ECS, no observers
5. **Logging is first-class** — every physics decision is traceable through `parcour.log`

---

## 4. Source Code Organization

The source tree is split into **engine** (reusable, game-agnostic systems) and **game** (title-specific logic). This separation means the engine modules could power a different game with no changes.

```
Parcour/
├── build.bat               Build script (release, debug, test, clean)
├── src/
│   ├── version.h           Shared version constants for all binaries (1.1.0.0)
│   ├── engine/             Reusable engine systems → compiled into engine.lib
│   │   ├── types.h         Root header — display constants, tile sizes, bounding box
│   │   ├── math.c/.h       Pure math utilities (abs, clamp, lerp, sign, min, max)
│   │   ├── gravity.c/.h    Gravity model with air drag + landing classification
│   │   ├── physics.c/.h    Velocity helpers + tile-map collision resolution
│   │   ├── input.c/.h      Keyboard state tracking with edge detection
│   │   ├── renderer.c/.h   Framebuffer ownership + drawing primitives
│   │   ├── log.c/.h        File-based debug logging with sub-ms timestamps
│   │   ├── levelfile.c/.h  .parcour file format (parser, validator, writer)
│   │   ├── levelgen.c/.h   Random maze level generator with connectivity checks
│   │   └── stb_image.h     Third-party: single-header PNG decoder (public domain)
│   ├── game/               Game-specific logic → links against engine.lib
│   │   ├── game.c          Entry point, window creation, game loop, CLI level loading
│   │   ├── character.c/.h  Character state machine + sprite animation driver
│   │   ├── sprite.c/.h     PNG loading (resources + disk) + alpha-blended blitting
│   │   ├── level.c/.h      Tile map data + collision queries + tile rendering
│   │   ├── resource.h      Resource IDs for embedded sprite PNGs
│   │   ├── sprites.rc      Win32 resource script embedding all 44 PNGs
│   │   └── version_game.rc VERSIONINFO resource for parcour.exe
│   └── editor/             Standalone level editor (Win32 GUI)
│       ├── editor.c        Tile painting, save/load .parcour, clearance warnings
│       └── version_editor.rc VERSIONINFO resource for editor.exe
├── levels/                 Custom level files (.parcour format)
│   ├── test3.parcour       Sealed room (enclosure validation test)
│   ├── test4.parcour       Low ceiling
│   ├── test5.parcour       Single-tile walls
│   └── test6.parcour       Staircase (auto-step-up test)
├── test/                   Automated test harness (18 files, 81 tests)
│   ├── test_framework.h    Macros (TEST_BEGIN, ASSERT_*, TEST_PASS) + extern globals
│   ├── test_helpers.h/.c   Shared helpers (simulate, replay, invariant checker)
│   ├── test_main.c         Entry point — parses --quick/--full, calls run_*_tests()
│   ├── version_test.rc     VERSIONINFO resource for test_runner.exe
│   ├── engine/             Engine module tests (link against engine.lib)
│   │   ├── test_math.c     5 tests — math utilities
│   │   ├── test_gravity.c  4 tests — gravity model
│   │   ├── test_physics.c  6 tests — collision resolution
│   │   ├── test_levelfile.c 7 tests — .parcour parsing, validation, roundtrip
│   │   └── test_levelgen.c 9 tests — maze gen, connectivity, enclosure detection
│   └── game/               Game module tests (link against engine.lib + game sources)
│       ├── test_character.c       8 tests — character FSM core
│       ├── test_state_transitions.c  state transition edge cases + wall-jump
│       ├── test_level.c           5 tests — tile queries
│       ├── test_regression.c      regression tests for specific bugs
│       ├── test_sweep.c           position sweeps with body-overlap pre-checks
│       ├── test_input.c           4 tests — keyboard state
│       ├── test_renderer.c        3 tests — framebuffer
│       ├── test_sprite.c          3 tests — animation ticking
│       ├── test_animation.c       7 tests — speed, looping, one-shot
│       └── test_integration.c     9 tests — multi-system flows
├── doc/                    Documentation
│   └── DESIGN.md           This document
├── sprites/                Pre-rendered PNG sprite frames (rvros asset pack)
│   ├── adventurer-idle-00.png .. 03.png
│   ├── adventurer-walk-00.png .. 05.png
│   ├── adventurer-run-00.png  .. 05.png
│   ├── adventurer-jump-00.png .. 03.png
│   ├── adventurer-fall-00.png .. 01.png
│   ├── adventurer-smrslt-00.png .. 03.png
│   ├── adventurer-crnr-grb-00.png .. 03.png
│   ├── adventurer-crnr-clmb-00.png .. 04.png
│   ├── adventurer-wall-slide-00.png .. 01.png
│   ├── adventurer-stand-00.png .. 02.png
│   ├── adventurer-crouch-00.png .. 03.png
│   └── adventurer-slide-*.png (mapped via crouch)
└── build/                  Build output (architecture-specific)
    ├── amd64fre/            engine.lib, sprites.res, parcour.exe, test_runner.exe, .obj
    └── amd64chk/            engine.lib, sprites.res, parcour.exe, test_runner.exe, .pdb, .ilk
```

### Engine vs Game Separation

| Layer | Directory | Modules | Build Output |
|-------|-----------|---------|-------------|
| **Engine** | `src/engine/` | types, math, gravity, physics, input, renderer, log, levelfile, levelgen | `engine.lib` (static library) |
| **Game** | `src/game/` | game, character, level, sprite | `parcour.exe` (links `engine.lib` + `sprites.res` + `version_game.res`) |
| **Editor** | `src/editor/` | editor | `editor.exe` (links `engine.lib` + `version_editor.res`) |
| **Tests** | `test/` | test_main, test_helpers, 14 test files | `test_runner.exe` (links `engine.lib` + `version_test.res`) |

**Dependency direction**: Game → Engine (never the reverse). Engine modules only depend on other engine modules. The one intentional coupling is `physics.c` including `level.h` for tile-query functions (`tile_is_solid`); this interface could be abstracted behind function pointers if the engine were extracted into a fully standalone library.

### Build Pipeline

The build proceeds in four stages, each depending on the previous:

```
Stage 1: engine.lib     Stage 2: sprites.res     Stage 2b: version *.res    Stage 3: parcour.exe       Stage 4: test_runner.exe
┌─────────────────┐     ┌──────────────────┐     ┌────────────────────┐     ┌─────────────────────┐    ┌──────────────────────┐
│ cl /c engine/*.c │──▸  │ rc sprites.rc    │──▸  │ rc version_*.rc    │──▸  │ cl game/*.c          │    │ cl test/**/*.c        │
│ lib /out:engine  │     │ → sprites.res    │     │ → version_game.res │     │ + engine.lib         │    │ + game/{char,spr,lvl} │
│                  │     └──────────────────┘     │ → version_test.res │     │ + sprites.res        │    │ + engine.lib          │
└─────────────────┘                               │ → version_editor   │     │ + version_game.res   │    │ + version_test.res    │
                                                  └────────────────────┘     │ + user32,gdi32,winmm │    └──────────────────────┘
                                                                             └─────────────────────┘
```

- **Stage 1**: Engine sources are compiled to `.obj` files, then archived into `engine.lib` using `lib.exe`. This produces a reusable static library with no game dependencies.
- **Stage 2**: The resource compiler (`rc.exe`) embeds all 44 sprite PNGs into `sprites.res` as `RCDATA` resources. Each PNG is assigned a numeric ID defined in `resource.h`, with animations grouped in blocks of 16 (e.g., `IDR_IDLE` = 164, frames 164–167).
- **Stage 2b**: Version info resources are compiled from `.rc` files that `#include "version.h"` for shared version constants (1.1.0.0). Each binary gets its own VERSIONINFO with file description, product name, and copyright. Debug builds define `_DEBUG` to set the `VS_FF_DEBUG` flag.
- **Stage 3**: Game sources (game.c, character.c, sprite.c, level.c) are compiled and linked against `engine.lib`, `sprites.res`, `version_game.res`, and Win32 system libraries to produce `parcour.exe`. The result is a **single self-contained executable** (~255 KB) with all sprites and version info embedded — no loose files needed for deployment.
- **Stage 4**: Test sources and game modules (character, sprite, level — but **not** game.c) are compiled and linked against the **same** `engine.lib` and `version_test.res` to produce `test_runner.exe`. Tests do not need sprites.res since they exercise logic, not rendering.

### Test Directory Organization

Tests mirror the source tree split:

| Directory | Files | What's Tested |
|-----------|------:|---------------|
| `test/` | 4 | Shared infrastructure: entry point, framework macros, helper functions |
| `test/engine/` | 5 | Engine modules: math (5), gravity (4), physics (6), levelfile (7), levelgen (9) |
| `test/game/` | 9 | Game modules: character (8), state transitions, level (5), regression, sweep, input (4), renderer (3), sprite (3), animation (7), integration (9) |

**Build system**: The compiler receives `/I"src/engine"` and `/I"src/game"` as include paths, so all `#include` directives use bare filenames (e.g., `#include "math.h"`) regardless of which sub-folder the file lives in. Test files additionally get `/I"test"` for the shared test headers.

### Lines of Code (approximate)

| File | Layer | Lines | Purpose |
|------|-------|------:|---------|
| `engine/types.h` | Engine | 39 | Shared constants |
| `engine/math.c` | Engine | 36 | Math utilities |
| `engine/log.c` | Engine | 62 | File logging |
| `engine/input.c` | Engine | 37 | Keyboard state |
| `engine/renderer.c` | Engine | 120 | Framebuffer + draw primitives |
| `engine/gravity.c` | Engine | 51 | Gravity model |
| `engine/physics.c` | Engine | 150 | Collision resolution (leading-edge + overlap-aware) |
| `engine/levelfile.c` | Engine | ~200 | .parcour file format parser/writer |
| `engine/levelgen.c` | Engine | ~180 | Random maze generator |
| `game/game.c` | Game | 220 | Window + game loop + CLI level loading |
| `game/character.c` | Game | 680 | State machine (largest file) |
| `game/sprite.c` | Game | 120 | PNG load + alpha blit |
| `game/level.c` | Game | 165 | Tile map + collision queries |
| `editor/editor.c` | Editor | ~400 | Level editor GUI |
| **Total** | | **~2,460** | Pure C, no generated code |

---

## 5. Module Reference

Modules are listed engine-first, then game, mirroring the dependency order: engine modules have no upward dependencies, while game modules build on top of them.

### 5.1 `engine/types.h` — Root Header [Engine]

The only file that includes `<windows.h>`. Defines all shared constants:

| Constant | Value | Purpose |
|----------|------:|---------|
| `SCREEN_W` | 1280 | Logical framebuffer width (pixels) |
| `SCREEN_H` | 960 | Logical framebuffer height (pixels) |
| `TILE_SIZE` | 32 | Tile dimensions (32×32 px) |
| `LEVEL_COLS` | 40 | Tile map columns (1280/32) |
| `LEVEL_ROWS` | 30 | Tile map rows (960/32) |
| `RENDER_W` | 64 | Character bounding box width |
| `RENDER_H` | 128 | Character bounding box height (4 tiles tall) |
| `FPS` | 60 | Target frame rate |
| `FRAME_MS` | 16 | Milliseconds per frame (1000/60) |

### 5.2 `engine/math.c` — Foundation Layer [Engine]

Six pure functions with zero dependencies. Used throughout the codebase:

- `absf(v)` — absolute value
- `clampf(v, lo, hi)` — clamp to range
- `signf(v)` — returns -1, 0, or +1
- `lerpf(a, b, t)` — linear interpolation
- `minf(a, b)` / `maxf(a, b)` — min/max

### 5.3 `engine/log.c` — Diagnostic Logging [Engine]

- **Init**: `game_log_init()` — opens `parcour.log` in write mode (truncates), starts a `QueryPerformanceCounter` timer
- **Write**: `game_log(fmt, ...)` — printf-style, prefixed with `[  12.345]` sub-millisecond timestamp, flushed immediately
- **Close**: `game_log_close()` — writes closing marker, closes file handle

Log output is the **primary debugging tool**. Every state transition, landing, wall-slide trigger, and corner-grab is logged with position, velocity, and ground state.

### 5.4 `engine/input.c` — Keyboard State [Engine]

Tracks four arrow keys as binary flags. Provides **edge detection** for Up and Down:

- `keyUpJustPressed` — true only on the frame Up is first pressed
- `keyDownJustPressed` — true only on the frame Down is first pressed

Edge detection is critical for jump triggering (crouch → jump requires `keyUpJustPressed`, not held state).

### 5.5 `engine/renderer.c` — Framebuffer [Engine]

Owns a static `UINT32 framebuf[SCREEN_W * SCREEN_H]` array (4.9 MB). Provides:

- `renderer_clear(color)` — fill entire buffer
- `renderer_put_pixel(x, y, color)` — bounds-checked single pixel
- `renderer_fill_rect(x, y, w, h, color)` — filled rectangle
- `renderer_fill_circle/ellipse` — rasterized shapes (used by legacy skeletal renderer)
- `renderer_draw_limb/limb_shaded` — tapered limb drawing (legacy, retained for reference)

The framebuffer is blitted to the window each frame via `StretchDIBits` with aspect-ratio letterboxing.

### 5.6 `game/sprite.c` — PNG Sprite System [Game]

- **Resource loading** (primary): `sprite_anim_load_rc(anim, baseId, speed, loop)` — loads frames from embedded Win32 `RCDATA` resources using `FindResource` → `LoadResource` → `LockResource` → `stbi_load_from_memory`. Frame IDs are `baseId + 0`, `baseId + 1`, etc. This is the default path in the shipped executable.
- **Disk loading** (fallback): `sprite_anim_load(anim, dir, prefix, speed, loop)` — scans `dir\prefix-00.png`, `prefix-01.png`, etc., up to 16 frames. Uses `stb_image` for RGBA decode from file. Used only if no embedded resources are found (e.g., running an exe built without sprites.res).
- **Dual-mode design**: `character_load_sprites()` tries resource loading first; if zero frames load, it falls back to disk loading. This means development builds without .rc still work as long as the `sprites/` folder is present.
- **Blitting**: `sprite_blit(frame, x, y, scale, flipH)` — per-pixel alpha-blended blit with integer scaling and horizontal flip. Two paths: fast opaque (a ≥ 250) and full alpha blend.
- **RGBA → ARGB conversion**: Both loaders share a `decode_rgba_to_argb` helper that converts stb_image's RGBA output to the Windows BITMAPINFO ARGB format.

### 5.7 `engine/gravity.c` — Gravity Model [Engine]

Implements realistic free-fall with quadratic air drag:

```
vy_next = (vy + GRAV_ACCEL) * (1 - GRAV_DRAG * vy)   [falling only]
```

- **Rising** (vy < 0): Pure constant acceleration — preserves jump height predictability
- **Falling** (vy > 0): Air drag slows acceleration progressively — short falls feel snappy, tall falls visibly accelerate
- **Terminal velocity**: Approached asymptotically via drag (hard cap at `GRAV_TERMINAL = 18.0` as safety)

**Landing classification** (`gravity_landing_tier`):

| Tier | Condition | Response |
|------|-----------|----------|
| 0 — Soft | vy < 4.0 | Instant recovery |
| 1 — Medium | 4.0 ≤ vy < 8.0 | Brief stagger animation |
| 2 — Hard | vy ≥ 8.0 | Ninja roll (somersault + forward momentum) |

### 5.8 `game/level.c` — Tile Map [Game]

A static 40×30 integer grid where `1 = solid`, `0 = air`. The level is designed as a physics test playground:

**Collision query functions:**

| Function | Purpose |
|----------|---------|
| `tile_solid(px, py)` | Is the tile at pixel position solid? (OOB returns 1) |
| `tile_wall(px, py)` | Is it a **wall** (solid + vertically adjacent solid)? Used for wall-slide trigger |
| `tile_wall_beside(x, y, facing)` | Is there a wall next to the character? |
| `tile_ledge_nearby(x, y, facing, &grabY)` | Airborne corner-grab probe |
| `tile_head_ledge(x, y, facing, &grabY)` | Grounded head-height grab probe |

**Note:** `tile_wall()` is no longer used for horizontal collision (see §8.3). Horizontal collision now uses `tile_solid()` at all body heights with overlap-aware filtering. `tile_wall()` remains for wall-slide triggering.

### 5.9 `engine/physics.c` — Collision Resolution [Engine]

Three collision passes run every frame in order:

1. **Horizontal** (`physics_collide_horizontal`) — Leading-edge-only checks using `tile_solid()` at 5 body heights (head, upper, mid, lower, foot). Only blocks when the character would enter a **new** solid tile not already overlapped at the current position. Returns `hitWall` flag.
2. **Vertical** (`physics_collide_vertical`) — Center-foot-only landing detection. Character must have their center of mass over solid ground to land. Calls ceiling collision if rising.
3. **Ceiling** (`physics_collide_ceiling`) — Sweeps across all tile columns overlapped by the character's width. Stops upward velocity if any tile directly above the head is solid.

### 5.10 `engine/levelfile.c` — Level File Format [Engine]

Handles reading/writing the `.parcour` binary file format:
- **Parsing**: `levelfile_load(path, level_out)` — reads 40×30 tile grid from `.parcour` file
- **Validation**: checks magic number, dimensions, spawn clearance (2×4 tiles of air at spawn)
- **Writing**: `levelfile_save(path, level)` — serializes tile grid with header

### 5.11 `engine/levelgen.c` — Random Level Generator [Engine]

Generates random maze-like levels with guaranteed properties:
- **Connectivity**: all air regions are reachable (flood-fill verification)
- **Enclosure**: boundary walls on all four sides (row 0, row 29, col 0, col 39)
- **Spawn clearance**: guaranteed 2×4 air region at spawn point

### 5.12 `game/character.c` — State Machine [Game]

The heart of the project. A 15-state FSM that processes input, drives animation, and coordinates with physics. See §9 for the full state diagram.

---

## 6. Data Structures

### Character

```c
typedef struct {
    float x, y;            // top-left of bounding box (pixels)
    float vx, vy;          // velocity (pixels/frame)
    CharState state;        // current FSM state (enum, 0..14)
    int facing;            // +1 = right, -1 = left
    int animFrame;         // current animation frame index
    int animTick;          // ticks since last frame advance
    int onGround;          // 1 if standing on solid ground
    int stateTimer;        // frames since entering current state
    int jumpHoldFrames;    // frames of held jump (variable jump height)
    float landingVy;       // vy at moment of landing (for tier classification)
    int grabCooldown;      // frames before corner-grab can re-trigger
    int stepCooldown;      // frames before auto-step-up can re-trigger (20f)
} Character;
```

### SpriteFrame / SpriteAnim

```c
typedef struct {
    UINT32 *pixels;    // ARGB pixel data (heap-allocated at load time)
    int w, h;          // original frame dimensions (typically 50×37)
} SpriteFrame;

typedef struct {
    SpriteFrame frames[MAX_ANIM_FRAMES];  // up to 16 frames
    int count;         // number of frames loaded
    int speed;         // ticks per frame (lower = faster)
    int loop;          // 1 = loop, 0 = hold last frame
} SpriteAnim;
```

---

## 7. Game Loop and Frame Pipeline

```
WinMain(argc: optional .parcour file path)
  ├── game_log_init()
  ├── character_load_sprites()    ← load all PNG sprite sheets
  ├── character_init()            ← set starting position
  ├── if (argv[1]) levelfile_load(argv[1])  ← load custom level from CLI
  ├── CreateWindowW()             ← WS_OVERLAPPEDWINDOW, resizable + maximizable
  │
  └── LOOP (16ms / 60fps):
        ├── PeekMessageW()        ← process Windows messages (input, resize, close)
        ├── input_update()        ← edge-detect keyUpJustPressed / keyDownJustPressed
        ├── character_update()    ← FSM + physics + collision (THE BIG ONE)
        │     ├── Process input against current state
        │     ├── Apply gravity (gravity_apply)
        │     ├── physics_collide_horizontal → hitWall
        │     ├── Post-collision: wall-slide trigger, head-ledge grab
        │     ├── physics_collide_vertical → landing detection
        │     ├── Landing tier classification (gravity_landing_tier)
        │     ├── Fell-off-ledge detection
        │     └── Advance animation frame
        ├── render_frame()
        │     ├── renderer_clear()
        │     ├── level_draw()
        │     └── character_draw()
        └── StretchDIBits()       ← blit framebuffer to window (letterboxed)
```

### Window Resizing

The window supports maximize and manual resize. The framebuffer stays at a fixed logical resolution (1280×960) and is scaled to the window client area with **aspect-ratio-preserving letterboxing**:

```c
float scale = min(winClientW / SCREEN_W, winClientH / SCREEN_H);
int dstW = SCREEN_W * scale;
int dstH = SCREEN_H * scale;
// Center in window, fill margins with black
```

Minimum window size is enforced at 640×480 via `WM_GETMINMAXINFO`.

---

## 8. Physics System

### 8.1 Velocity Model

| Parameter | Value | Effect |
|-----------|------:|--------|
| `GROUND_ACCEL` | 0.55 | Horizontal acceleration per frame (grounded) |
| `GROUND_FRICTION` | 0.82 | Velocity multiplier per frame (grounded) |
| `AIR_CONTROL` | 0.08 | Horizontal acceleration per frame (airborne) |
| `AIR_FRICTION` | 0.98 | Velocity multiplier per frame (airborne) |
| `MAX_RUN_SPEED` | 6.0 | Hard cap on horizontal velocity |
| `STOP_THRESHOLD` | 0.05 | Below this, velocity snaps to zero |
| `JUMP_IMPULSE` | -10.0 | Instantaneous upward velocity on jump |
| `JUMP_HOLD_BOOST` | 0.35 | Additional upward velocity per held frame |
| `JUMP_HOLD_MAX` | 10 | Maximum frames of hold boost |

**Walk vs. Run**: Walking caps at `MAX_RUN_SPEED * 0.33` with half acceleration. After 30 frames of sustained walking, transitions to RUNNING at full speed.

**Variable-height jump**: Holding Up after jumping adds `JUMP_HOLD_BOOST` per frame for up to `JUMP_HOLD_MAX` frames. A tap gives a short hop; a hold gives maximum height.

### 8.2 Collision Probes

```
Character Bounding Box (64 × 128 px)
┌──────────────────────────────────┐  ← y
│  headL(+10)         headR(-11)   │  ← headY = y + 4
│                                  │
│                                  │  ← upperY = y + H/4
│                                  │
│  midL(+8)           midR(-9)     │  ← midY = y + H/2
│                                  │
│  lowL(+8)           lowR(-9)     │  ← lowerY = y + 3H/4
│                                  │
│          footC(+W/2)             │  ← footY = y + H
└──────────────────────────────────┘
```

**Horizontal collision** probes the leading edge at 5 heights (head, upper, mid, lower, foot) using `tile_solid()` with overlap-aware filtering.  
**Vertical collision** uses only `footC` (center foot) with `tile_solid()`.  
**Ceiling collision** sweeps all tile columns overlapped by the character's width at `headY`.

### 8.3 Full-Body Horizontal Collision with Overlap-Aware Filtering

The horizontal collision system evolved through 6 iterations (see §17.7). The current design uses **leading-edge-only** checks at **all 5 body heights** with an **overlap-aware filter** that prevents the character from getting stuck when already inside a solid tile (e.g., under a ceiling platform):

```c
int physics_collide_horizontal(float *x, float *vx, float y) {
    float newX = *x + *vx;
    int leadingNew = (*vx > 0) ? (int)(newX + RENDER_W - 1) : (int)newX;
    int leadingCur = (*vx > 0) ? (int)(*x + RENDER_W - 1) : (int)*x;

    float heights[] = { y + 4, y + H/4, y + H/2, y + 3*H/4, y + H - 1 };
    for (int i = 0; i < 5; i++) {
        if (tile_solid(leadingNew, heights[i])) {
            // Only block if entering a NEW solid tile not already overlapped
            if (!tile_solid(leadingCur, heights[i])) {
                *vx = 0;
                return 1;  // hitWall
            }
        }
    }
    *x = newX;
    return 0;
}
```

**Key design decisions:**

1. **Leading-edge-only**: Only the edge in the direction of movement is checked. This prevents the trailing edge from blocking movement when the character legitimately overlaps solid tiles behind them.

2. **All 5 body heights**: Unlike the earlier mid+low+foot-only approach, checking at all heights (including head) prevents the character from walking through walls that are only 1 tile wide at head level.

3. **Overlap-aware filter**: The critical innovation — when a tile IS solid at the new position, we also check if the CURRENT position is solid at the same height. If yes, the character is already overlapping that solid (e.g., a ceiling platform above their head) and blocking would trap them. Only genuinely NEW solid-tile entries are blocked.

**Why this is needed**: After corner-climbing to a position under ceiling tiles, the character's head overlaps those ceiling tiles. Without the overlap filter, horizontal movement would be blocked in both directions (head probe hits solid everywhere), permanently trapping the character.

### 8.4 Barrier Interaction Tiers

When the character walks into a barrier while grounded, the response depends on barrier height:

| Barrier Height | Body Probe | Response |
|---------------|------------|----------|
| **Foot-level** (1 tile) | Only foot probe hits | Auto step-up: teleport up 1 tile + IDLE. 20-frame cooldown prevents staircase speed-running |
| **Chest-level** (2-3 tiles) | Mid or lower probe hits | Hard stop: velocity zeroed, character must manually jump |
| **Head-level** (wall at head height) | Head or upper probe hits, with ledge nearby | Pull-up: triggers `CORNER_GRAB` → `CORNER_CLIMB` |

This creates a natural Prince of Persia feel where small obstacles are stepped over automatically, medium barriers require intentional jumps, and tall walls trigger the pull-up animation.

### 8.5 Center-Foot Landing Rule

Landing requires the character's **center** (x + RENDER_W/2) to be over solid ground. If the center of mass is past the edge, the character falls. This prevents the unrealistic "standing on air with one foot" artifact and eliminates edge-standing oscillation bugs.

---

## 9. Character State Machine

### State Diagram

```
                    ┌─────────────────────────────┐
                    │           IDLE               │
                    │  (friction, wait for input)  │
                    └──┬───┬───┬───────────────────┘
                       │   │   │
              Left/Right│  Up  │Down
                       │   │   │
                       ▼   ▼   ▼
                    ┌──────┐ ┌────────┐
                    │ WALK │ │ CROUCH │──(Up held)──→ JUMP_RISE
                    └──┬───┘ └────────┘
                       │ 30 frames
                       ▼
                    ┌─────────┐
                    │ RUNNING │──(Up)──→ SOMERSAULT
                    │         │──(Down)──→ SLIDE
                    └────┬────┘
                         │ reverse dir
                         ▼
                    ┌─────────┐
                    │ TURNING │──(timer)──→ IDLE/WALK
                    └─────────┘

     ┌──────────────── AIRBORNE ────────────────────┐
     │                                               │
     │  JUMP_RISE ──(vy≥0)──→ JUMP_FALL             │
     │  SOMERSAULT ──(vy≥0)──→ JUMP_FALL             │
     │                                               │
     │  JUMP_FALL ──(near ledge)──→ CORNER_GRAB      │
     │  JUMP_FALL ──(into wall)──→ WALL_SLIDE        │
     │                                               │
     │  WALL_SLIDE ──(Up)──→ JUMP_RISE (wall-jump)   │
     │  WALL_SLIDE ──(release)──→ JUMP_FALL          │
     │  WALL_SLIDE ──(near ledge)──→ CORNER_GRAB     │
     │                                               │
     └──── landing ──→ IDLE / LANDING / HARD_LANDING ─┘

     CORNER_GRAB ──(Up or hold forward 15f)──→ CORNER_CLIMB
     CORNER_GRAB ──(Down or away)──→ JUMP_FALL
     CORNER_CLIMB ──(20 frames)──→ IDLE (repositioned on top)

     Head-Ledge Grab (grounded + walking into head-height platform):
     WALK/RUNNING/STOPPING ──(tile_head_ledge)──→ CORNER_GRAB

     Auto-Step-Up (grounded + walking into foot-level barrier):
     WALK/RUNNING ──(foot-only hitWall + stepCooldown==0)──→ IDLE (y -= TILE_SIZE)
```

### State Table

| State | Animation | Loop | Speed | Grounded | Notes |
|-------|-----------|:----:|------:|:--------:|-------|
| `IDLE` | `adventurer-idle` | ✅ | 8 | ✅ | Default resting state |
| `WALK` | `adventurer-walk` | ✅ | 9 | ✅ | Slow movement, transitions to RUNNING after 30f |
| `RUNNING` | `adventurer-run` | ✅ | 3 | ✅ | Full speed |
| `STOPPING` | `adventurer-stand` | ❌ | 4 | ✅ | Deceleration from run |
| `TURNING` | `adventurer-stand` | ❌ | 3 | ✅ | Direction reversal |
| `CROUCH` | `adventurer-crouch` | ❌ | 3 | ✅ | Wind-up before jump |
| `JUMP_RISE` | `adventurer-jump` | ❌ | 5 | ❌ | Ascending phase |
| `JUMP_FALL` | `adventurer-fall` | ✅ | 6 | ❌ | Descending phase |
| `LANDING` | `adventurer-smrslt` | ❌ | 3 | ✅ | Medium-impact recovery (8 frames) |
| `CORNER_GRAB` | `adventurer-crnr-grb` | ❌ | 6 | ✅* | Hanging on ledge edge |
| `CORNER_CLIMB` | `adventurer-crnr-clmb` | ❌ | 4 | ✅* | Pulling up onto ledge (20 frames) |
| `WALL_SLIDE` | `adventurer-wall-slide` | ✅ | 6 | ❌ | Slow descent against wall |
| `SOMERSAULT` | `adventurer-smrslt` | ✅ | 3 | ❌ | Running jump flip |
| `SLIDE` | `adventurer-slide` | ❌ | 4 | ✅ | Ground slide from run |
| `HARD_LANDING` | `adventurer-smrslt` | ❌ | 3 | ✅ | Ninja roll on hard impact (18 frames) |

*CORNER_GRAB and CORNER_CLIMB set `onGround=1` to simplify state machine routing, though the character is visually hanging.

### PoP-Style Ledge Mechanics

**1. Airborne Corner-Grab**: While falling (`JUMP_FALL` or `WALL_SLIDE`), if a ledge is detected within reach in the facing direction, the character snaps to the grab position. A `grabCooldown` of 15-20 frames prevents oscillation.

**2. Grounded Head-Ledge Grab**: While walking/running into a head-height platform, if `tile_head_ledge()` detects a platform edge at head level, the character transitions to `CORNER_GRAB`. No `hitWall` required (thin platforms don't trigger horizontal collision).

**3. Auto-Climb**: While in `CORNER_GRAB`, if the player keeps holding the forward arrow for 15 frames (~0.5s), the character automatically transitions to `CORNER_CLIMB` — no Up press needed. Pressing Up still climbs instantly.

**4. Corner-Climb Exit**: After 20 frames of climb animation, the character is repositioned:
- Horizontal: `x += facing * RENDER_W` (64px forward)
- Vertical: `y -= RENDER_H * 3/4` (96px up)
- Safety nudge: shifts 1px at a time until center foot is on solid ground

---

## 10. Level System

### Tile Map Structure

The level is a 40×30 grid of integers stored as `int level[LEVEL_ROWS][LEVEL_COLS]`. Currently only two tile types exist: `0` (air) and `1` (solid).

### Default Level Layout

The built-in level is designed specifically to exercise all physics mechanics:

| Region | Columns | Rows | Purpose |
|--------|---------|------|---------|
| **Boundary Walls** | 0, 39 | all | Full-height solid walls enclosing the map |
| **Boundary Ceiling/Floor** | all | 0, 29 | Full-width solid ceiling and floor |
| **Left Tower** | 0-5 | 3-26 | Tall structure with cliff drop for hard-landing testing |
| **Center Stairs** | 9-24 | 10-22 | Staggered platforms at various heights for jump testing |
| **Right Ledge Chain** | 33-37 | 3,7,11,15,19,23 | Ascending thin platforms every 4 rows for corner-grab/climb testing |
| **Right Wall** | 37 | 3-26+ | Continuous wall column for wall-slide testing |
| **Ground Platforms** | 0-7, 31-39 | 26 | Elevated ground sections with gap (hard-landing test) |
| **Col 38** | 38 | most | Air gap between platform edges and boundary wall |

### `.parcour` File Format

Custom levels use a binary `.parcour` file format:
- **Header**: magic number + 40×30 dimensions + spawn point coordinates
- **Body**: 1200 bytes (40×30 tile grid, 1 byte per tile)
- **Validation on load**: checks enclosure (solid boundary), spawn clearance (2×4 air at spawn)

**CLI usage**: `parcour.exe mymap.parcour` loads a custom level instead of the default.

### Level Editor

A standalone Win32 GUI application (`editor/editor.c`) for creating `.parcour` files:
- **Tile painting**: Left-click to place solid, right-click to erase
- **Save/Load**: Saves/loads `.parcour` files via standard file dialogs
- **Clearance warnings**: Highlights areas where the 2×4 character body wouldn't fit
- **Enclosure validation**: Warns if boundary walls are incomplete
- **Grid overlay**: Shows tile boundaries for precise placement

### Random Level Generator

`levelgen.c` generates random maze-like levels with guaranteed properties:
- All air regions are connected (flood-fill verification)
- Boundary walls on all four sides
- Spawn point has guaranteed 2×4 tile clearance
- Used by test harness for randomized collision testing

### Ledge Probe Functions

**`tile_ledge_nearby`** (airborne grab):
- Probes at `x + RENDER_W + 2` (facing right) or `x - 3` (facing left)
- Scans a vertical window from character top to top + H/3
- Looks for pattern: solid tile with air above = ledge

**`tile_head_ledge`** (grounded grab):
- Probes same horizontal offset
- Checks at head height: `y + RENDER_H/4`
- Same pattern: solid at probe point, air above it

---

## 11. Sprite and Rendering Pipeline

### Asset Pipeline

Sprites are embedded as Win32 resources at build time and decoded at runtime:

```
Build time:                             Runtime:
PNG files (50×37 px, RGBA)              FindResource(IDR_IDLE + n)
    ↓ rc.exe                                ↓ LoadResource / LockResource
sprites.res (RCDATA blobs)              Raw PNG bytes in memory
    ↓ link.exe                              ↓ stbi_load_from_memory
parcour.exe (.rsrc section)          RGBA byte array
                                            ↓ decode_rgba_to_argb (sprite.c)
                                        ARGB UINT32 array (SpriteFrame.pixels)
                                            ↓ sprite_blit (per-frame)
                                        framebuf[SCREEN_W × SCREEN_H]  (ARGB, software composited)
                                            ↓ StretchDIBits (GDI)
                                        Window client area (hardware-scaled with letterbox)
```

**Fallback path**: If resources are not present (e.g., test builds or development exe without .rc), the loader falls back to reading loose PNG files from a `sprites/` directory on disk using `stbi_load`.

### Resource Embedding

All 44 sprite PNGs are embedded in `sprites.rc` as numbered `RCDATA` resources. Resource IDs are defined in `resource.h` with each animation assigned a base ID spaced 16 apart (matching `MAX_ANIM_FRAMES`):

| Animation | Base ID | Frames | Resource IDs |
|-----------|---------|--------|-------------|
| `adventurer-crnr-clmb` | 100 | 5 | 100–104 |
| `adventurer-crnr-grb` | 116 | 4 | 116–119 |
| `adventurer-crouch` | 132 | 4 | 132–135 |
| `adventurer-fall` | 148 | 2 | 148–149 |
| `adventurer-idle` | 164 | 4 | 164–167 |
| `adventurer-jump` | 180 | 4 | 180–183 |
| `adventurer-run` | 196 | 6 | 196–201 |
| `adventurer-smrslt` | 212 | 4 | 212–215 |
| `adventurer-stand` | 228 | 3 | 228–230 |
| `adventurer-walk` | 244 | 6 | 244–249 |
| `adventurer-wall-slide` | 260 | 2 | 260–261 |

Some character states share the same sprite animation (e.g., LANDING, SOMERSAULT, SLIDE, and HARD_LANDING all use `IDR_SMRSLT` with different speed/loop settings).

### Sprite Naming Convention

Files follow the pattern `adventurer-{action}-{NN}.png` where NN is a zero-padded frame number. The loader scans sequentially until a file is not found.

### Alpha Blending

Two-path blending in `sprite_blit`:
1. **Fast path** (alpha ≥ 250): Direct pixel write — treats as fully opaque
2. **Blend path** (alpha < 250): Per-channel `src * a + dst * (255 - a)` / 255

At 3× scale, each source pixel maps to a 3×3 block, so the 50×37 sprite renders at 150×111 pixels on screen.

---

## 12. Logging and Telemetry

### Log Format

```
=== Parcour Debug Log ===

[   0.000] Game starting up
[   0.015] Sprites loaded: 55 frames from C:\...\sprites
[   0.016] Player init at (320, 800)
[   5.234] STATE: IDLE -> WALK  (pos=320.0,800.0 vel=0.00,0.00 ground=1)
[   6.890] STATE: WALK -> RUNNING  (pos=380.5,800.0 vel=1.82,0.00 ground=1)
[   7.123] HEAD-LEDGE GRAB at grabY=800.0 (was y=800.0, grounded, walking into wall)
[   7.123] STATE: RUNNING -> CORNER_GRAB  (pos=928.0,800.0 vel=0.00,0.00 ground=1)
[   7.373] STATE: CORNER_GRAB -> CORNER_CLIMB  (pos=928.0,800.0 vel=0.00,0.00 ground=1)
```

### What Gets Logged

| Event | Information Included |
|-------|---------------------|
| State transition | Old state → new state, position, velocity, ground flag |
| Landing | `landingVy`, tier (0/1/2), estimated fall height in pixels, horizontal velocity |
| Wall-slide trigger | Source (pre-collision or post-collision), hitWall flag, inputDir, vy |
| Corner-grab | grabY, previous y, vy at grab moment |
| Head-ledge grab | grabY, previous y, context (grounded, walking) |
| Startup | Sprite count, sprite directory path |
| Shutdown | Clean shutdown marker |

### Using Logs for Debugging

```powershell
# View last 50 lines of the log
Get-Content parcour.log -Tail 50

# Find all state transitions
Select-String "STATE:" parcour.log

# Find all landings
Select-String "LANDED:" parcour.log

# Find corner-grab events
Select-String "CORNER-GRAB|HEAD-LEDGE" parcour.log

# Track a specific position range
Select-String "pos=92[0-9]" parcour.log
```

---

## 13. Build System

### Prerequisites

| Tool | Required Version | Purpose |
|------|-----------------|---------|
| **Microsoft Visual Studio** | 2019 or later (any edition) | MSVC compiler (`cl.exe`), linker (`link.exe`), librarian (`lib.exe`), resource compiler (`rc.exe`) |
| **Windows SDK** | 10.0+ (ships with VS) | `windows.h`, GDI, WinMM |

No other dependencies. No package managers. No CMake. No vcpkg.

### Build Script (`build.bat`)

The project uses a single `build.bat` script at the project root that supports multiple configurations:

```
build.bat              Build release (amd64fre) + run tests
build.bat fre          Build release (amd64fre) + run tests
build.bat chk          Build debug   (amd64chk) + run tests
build.bat all          Build both configs + run tests
build.bat clean        Delete all build artifacts
build.bat notest       Build release without running tests
```

**What the script does:**
1. Auto-detects Visual Studio installation (checks cl.exe in PATH, then VS 2022/2019/Insiders)
2. Creates `build/amd64fre/` and `build/amd64chk/` directories if needed
3. Compiles engine sources into `engine.lib` (static library)
4. Compiles `sprites.rc` into `sprites.res` (embeds all 44 sprite PNGs)
5. Links game sources + `engine.lib` + `sprites.res` + Win32 libs into `parcour.exe`
6. Links test sources + game modules + `engine.lib` into `test_runner.exe`
7. **Runs tests automatically** — build fails if any test fails
8. Reports pass/fail for both game build and test execution

### Build Configurations

| Config | Output Dir | Compiler Flags | Use Case |
|--------|-----------|---------------|----------|
| `amd64fre` (release) | `build/amd64fre/` | `/O2 /MT /W4 /WX` | Normal gameplay, performance testing |
| `amd64chk` (debug) | `build/amd64chk/` | `/Od /Zi /MTd /W4 /WX /D_DEBUG` | Debugging with full symbols, no optimization |

### Compiler Flags

| Flag | Purpose |
|------|---------|
| `/O2` | Full optimization (release) |
| `/Od` | Disable optimization (debug — easier to step through) |
| `/Zi` | Generate full debug info in PDB (debug) |
| `/MT` | Static link C runtime (release) — no vcruntime DLL dependency |
| `/MTd` | Static link debug C runtime (debug) — no vcruntime DLL dependency |
| `/W4` | Warning level 4 (near-maximum) |
| `/WX` | Treat warnings as errors |
| `/D_DEBUG` | Define `_DEBUG` preprocessor symbol (debug) |
| `/nologo` | Suppress compiler banner |
| `/I"src/engine"` | Include path for engine headers |
| `/I"src/game"` | Include path for game headers |
| `/I"test"` | Include path for test headers (test builds only) |

### Linked Libraries

| Library | API Surface |
|---------|-------------|
| `engine.lib` | Project's own engine (math, gravity, physics, input, renderer, log) |
| `sprites.res` | Compiled Win32 resources containing all 44 embedded sprite PNGs |
| `user32.lib` | Window creation, message loop, keyboard input |
| `gdi32.lib` | `StretchDIBits`, `FillRect`, DC management |
| `winmm.lib` | `timeGetTime` (frame timing) |

### Build from PowerShell (manual)

The four-stage build can be run manually after calling `vcvars64.bat`:

```powershell
# Stage 1: Build engine.lib
cd build\amd64fre
cl /c /O2 /MT /W4 /WX /nologo /I"..\..\src\engine" /I"..\..\src\game" ..\..\src\engine\gravity.c ..\..\src\engine\input.c ..\..\src\engine\log.c ..\..\src\engine\math.c ..\..\src\engine\physics.c ..\..\src\engine\renderer.c
lib /nologo /out:engine.lib gravity.obj input.obj log.obj math.obj physics.obj renderer.obj

# Stage 2: Build sprites.res (embed PNGs)
rc /nologo /I"..\..\src\game" /fo sprites.res "..\..\src\game\sprites.rc"

# Stage 3: Build parcour.exe (links engine.lib + sprites.res)
cl /O2 /MT /W4 /WX /nologo /I"..\..\src\engine" /I"..\..\src\game" ..\..\src\game\game.c ..\..\src\game\character.c ..\..\src\game\sprite.c ..\..\src\game\level.c /link engine.lib sprites.res user32.lib gdi32.lib winmm.lib /out:parcour.exe

# Stage 4: Build test_runner.exe (links same engine.lib, no sprites.res)
cl /O2 /MT /W4 /WX /nologo /I"..\..\src\engine" /I"..\..\src\game" /I"..\..\test" ..\..\test\test_main.c ..\..\test\test_helpers.c ..\..\test\engine\test_math.c ..\..\test\engine\test_gravity.c ..\..\test\engine\test_physics.c ..\..\test\game\test_character.c ..\..\test\game\test_state_transitions.c ..\..\test\game\test_level.c ..\..\test\game\test_regression.c ..\..\test\game\test_sweep.c ..\..\test\game\test_replay.c ..\..\test\game\test_integration.c ..\..\src\game\character.c ..\..\src\game\sprite.c ..\..\src\game\level.c /link engine.lib user32.lib gdi32.lib winmm.lib /out:test_runner.exe
```

### Common Build Issues

| Problem | Cause | Fix |
|---------|-------|-----|
| `fatal error C1034: stdio.h: no include path set` | `cl.exe` found but environment not initialized | Run `vcvars64.bat` first or use Developer Command Prompt |
| `error LNK1168: cannot open parcour.exe for writing` | Previous instance still running | `Get-Process parcour \| Stop-Process` |
| `/WX` treats warning as error | Unused variable or parameter | Use `(void)varname;` to suppress, or remove the variable |
| Tests fail with `FAIL:` output | Test assertion failed | Read the test output — shows file, line, and expected vs actual |

### Deployment

The release build produces a **single self-contained executable** (`parcour.exe`, ~255 KB) that can be copied to any 64-bit Windows machine and run with zero external dependencies:

- **C runtime**: Statically linked via `/MT` (release) and `/MTd` (debug). No `vcruntime140.dll` or Visual C++ Redistributable needed.
- **Sprites**: Embedded as Win32 `RCDATA` resources via `sprites.rc`. No loose PNG files required at runtime.
- **System libraries**: `user32.dll`, `gdi32.dll`, and `winmm.dll` are part of every Windows installation.

**To distribute**: copy `build\amd64fre\parcour.exe` — that's it. One file.

---

## 13a. Automated Test Harness

### Overview

The project includes a comprehensive headless test suite split across **18 source files** in `test/`, organized by module. Tests run as a console application (`test_runner.exe`) and are automatically executed by `build.bat`. The suite contains **81 tests** across 12 categories, including invariant checking, position sweeps, bug regression tests, level file format tests, and maze generation tests.

### Test File Organization

Test files mirror the `src/engine` and `src/game` split. Shared infrastructure stays at the `test/` root:

| File | Tests | Purpose |
|------|------:|---------|
| **test/** (shared) | | |
| `test_main.c` | — | Entry point; owns globals, parses `--quick`/`--full`, calls `run_*_tests()` |
| `test_framework.h` | — | Macros (`TEST_BEGIN`, `ASSERT_*`, `TEST_PASS`) and `extern` globals |
| `test_helpers.h` | — | Helper declarations, `InputFrame` struct, `REPEAT_INPUT`, `run_*_tests()` prototypes |
| `test_helpers.c` | — | Shared helper implementations (`simulate`, `replay`, `check_invariants`, etc.) |
| **test/engine/** | | |
| `test_math.c` | 5 | `absf`, `clampf`, `signf`, `lerpf`, `minf/maxf` |
| `test_gravity.c` | 4 | Acceleration, terminal velocity, air drag, landing tiers |
| `test_physics.c` | 6 | Vertical/horizontal collision, thin platforms, ceiling, overlap-aware filter |
| `test_levelfile.c` | 7 | `.parcour` file parsing, validation, roundtrip, error handling |
| `test_levelgen.c` | 9 | Maze generation, connectivity, enclosure detection, spawn clearance |
| **test/game/** | | |
| `test_character.c` | 8 | Core FSM: idle, walk, run, jump, fall, wall-slide, corner-grab, climb |
| `test_state_transitions.c` | — | Edge-case transitions: TURNING, SLIDE, SOMERSAULT, CROUCH, lockout, wall-jump |
| `test_level.c` | 5 | Tile queries, out-of-bounds, wall vs platform, head ledge |
| `test_regression.c` | — | Specific bug regressions: boundary wall block, grab cooldown, auto-climb, corner-climb escape |
| `test_sweep.c` | — | Position sweeps with body-overlap pre-checks |
| `test_input.c` | 4 | Keyboard state, edge detection |
| `test_renderer.c` | 3 | Framebuffer clear, pixel put, fill rect |
| `test_sprite.c` | 3 | Animation ticking, frame advance |
| `test_animation.c` | 7 | Speed, looping, one-shot, frame clamping |
| `test_integration.c` | 9 | Multi-system integration flows |

Each `test_*.c` file contains `static` test functions plus a single public `run_*_tests()` entry point called from `test_main.c`. All test executables link against the **same `engine.lib`** produced in Stage 1 of the build, ensuring tests exercise the identical engine binary that ships in the game.

### Test Modes

The test runner supports two modes, selectable via command-line flag:

| Mode | Flag | Tests | Use Case |
|------|------|------:|----------|
| **Quick** | `--quick` | ~25 | Math, gravity, level, physics, levelfile — no multi-frame simulation. Fast iteration. |
| **Full** | `--full` (default) | 81 | Everything: state machine, sweeps, regressions, level gen. CI/pre-commit. |

Build script usage:
```
build.bat fre              # full tests (default)
build.bat fre quick        # quick tests only
build.bat chk quick        # debug build + quick tests
```

### Test Categories (81 tests)

| Category | Tests | What's Covered |
|----------|------:|----------------|
| **Math** | 5 | absf, clampf, signf, lerpf, minf/maxf |
| **Gravity** | 4 | Acceleration, terminal velocity, air drag, landing tier classification |
| **Level** | 5 | Out-of-bounds, solid/air tiles, wall vs platform, wall_beside, head_ledge |
| **Physics** | 6 | Vertical landing, center-foot rule, wall blocking, thin platform pass-through, ceiling, overlap-aware filtering |
| **Level File** | 7 | .parcour parsing, invalid file rejection, roundtrip save/load, spawn validation |
| **Level Gen** | 9 | Maze generation, connectivity (flood fill), enclosure checks, spawn clearance |
| **Character FSM** | 8 | Idle, walk, run, stop, jump, fall, wall-slide, corner-grab, climb |
| **State Transitions** | — | TURNING flip, SLIDE, SOMERSAULT, CROUCH hold, HARD_LANDING lockout, wall-jump |
| **Regression** | — | Boundary wall block, grab cooldown oscillation, auto-climb, corner-climb escape |
| **Sweep / Property** | — | Position sweeps with body-overlap pre-checks, invariant validation |
| **Input / Renderer / Sprite** | 10 | Keyboard edge detection, framebuffer ops, animation ticking |
| **Animation** | 7 | Speed, looping, one-shot, frame clamping |
| **Integration** | 9 | Multi-system flows, friction stop, direction reversal, full action cycles |

### Invariant Checker

A `check_invariants()` function runs after every frame in `simulate_checked()` and replay tests. It catches physics bugs that would be invisible in point-tests:

| Invariant | What It Catches |
|-----------|----------------|
| `x` within world bounds | Character escaping the level |
| `y` within world bounds | Falling through the floor |
| `state` is valid enum | Corrupted state machine |
| `facing` is ±1 | Direction corruption |
| `onGround` + solid under foot | Floating in air, or onGround flag lies |

Any invariant violation prints the exact frame number, position, and state — making root-cause analysis trivial.

### Position Sweep Tests

Instead of testing one hand-picked position, sweep tests exercise the engine across many positions:

- **`test_sweep_freefall_always_lands`** — Drops the character from the top of every column (2–37). Every single one must land on ground within 300 frames.
- **`test_sweep_walk_right_never_stuck`** — Starts on 8 different platform rows and walks right. Must move or fall — never freeze.
- **`test_sweep_invariants_during_freefall`** — Runs the invariant checker during free-fall from 34 columns. Catches position-specific collision bugs.

### Deterministic Replay Tests

Record a known-good input sequence and replay it to verify the engine produces the same outcome. If physics or FSM logic changes, the replay test breaks — catching **any** behavioral regression.

```c
typedef struct { int dir; int jump; int down; } InputFrame;
InputFrame inputs[200];
// ... fill with known sequence ...
replay(&p, inputs, count);  // invariants checked every frame
```

### Test Framework

The test harness uses a lightweight custom framework (no external test library):

```c
#define ASSERT_TRUE(expr)               /* Fails with file:line if expr is false */
#define ASSERT_EQ_INT(exp, act)         /* Fails showing expected vs actual int */
#define ASSERT_EQ_FLOAT(exp, act, eps)  /* Float comparison with epsilon */
#define TEST_BEGIN(name)                /* Starts a named test */
#define TEST_PASS()                     /* Marks test as passed */
```

### Helper Functions

- **`simulate(p, frames, dir, jumpFrame)`** — Runs N frames of `character_update()` with given input
- **`simulate_checked(p, frames, dir, jumpFrame)`** — Same as `simulate` but runs `check_invariants()` every frame
- **`simulate_record(p, frames, dir, history)`** — Records state history for oscillation detection
- **`replay(p, inputs, count)`** — Replays a recorded input sequence with invariant checking
- **`detect_oscillation(history, count)`** — Checks for ABABAB state patterns (oscillation bugs)
- **`input_set_dir(dir)`** / **`input_clear()`** — Manipulate global input state directly

### How to Add a Test

1. Identify which layer your test belongs to — engine tests go in `test/engine/`, game tests in `test/game/`
2. Find the appropriate `test_*.c` file (e.g., physics → `test/engine/test_physics.c`)
3. Add a `static void test_my_thing(void)` function
4. Use `TEST_BEGIN("category: description")` at the start
5. Use `ASSERT_*` macros for verification
6. End with `TEST_PASS()`
7. Call it from the file's `run_*_tests()` function
8. If adding a new category, create a new file in the correct subfolder (`test/engine/` or `test/game/`), add a `run_newmodule_tests()` prototype to `test_helpers.h`, add the file to `TEST_SRCS` in `build.bat`, and call `run_newmodule_tests()` from `test_main.c`
9. If it needs invariant checking, use `simulate_checked()` or `replay()`
10. Rebuild — the test links against `engine.lib` and runs automatically

### Design Decisions

- **No mocking needed** — tests manipulate global input variables directly (`keyLeft`, `keyRight`, etc.)
- **Renderer is linked but no-op** — framebuffer writes go nowhere in headless mode; no crashes
- **Sprites not loaded** — `character_update()` works without loaded sprites (no visual output)
- **Console app** — uses `main()` not `WinMain`, no window creation
- **Exit code** — returns 0 on all pass, 1 on any failure (used by build script)
- **Quick/full split** — math and physics tests are fast and position-independent; FSM and sweep tests are thorough but slower. Quick mode enables sub-second iteration during rapid development.

---

## 14. Debugging Tips and Tricks

### 14.1 The Log Is Your Best Friend

The single most effective debugging technique is reading `parcour.log`. Every physics bug encountered during development was diagnosed by reading the log:

- **Oscillation loops** appear as rapid state transitions at the same position every frame
- **Getting stuck** appears as no state transitions despite input
- **False triggers** appear as unexpected state changes with logged probe values

### 14.2 Common Bug Patterns and How to Spot Them

**Pattern: Oscillation Loop**
```
[  45.001] STATE: IDLE -> JUMP_FALL  (pos=1192.9,480.0 ...)
[  45.017] STATE: JUMP_FALL -> IDLE  (pos=1192.9,480.0 ...)
[  45.033] STATE: IDLE -> JUMP_FALL  (pos=1192.9,480.0 ...)
```
*Cause*: Two systems disagree about ground state. Fix: single source of truth for landing detection.

**Pattern: Character Stuck**
```
[  30.500] STATE: WALK -> WALK  (pos=1064.7,224.0 ...)
[  30.517] STATE: WALK -> WALK  (pos=1064.7,224.0 ...)
```
*Cause*: Collision is blocking movement in all directions. Check which probes are hitting.

**Pattern: Premature Trigger**
```
[  12.345] WALL-SLIDE triggered (pre-collision probe)
```
*Cause*: Probe is detecting a thin platform as a wall. Check `tile_wall` vs `tile_solid` usage.

### 14.3 Adding Temporary Probes

To debug a specific collision issue, add targeted logging:

```c
game_log("DEBUG: footC=%d footY=%d solid=%d wall=%d",
         footC, footY, tile_solid(footC, footY), tile_wall(footC, footY));
```

Remember to remove debug logs after fixing — `/WX` will catch unused variables if you forget cleanup.

### 14.4 Kill Before Rebuild

The executable must not be running when the linker writes the output:

```powershell
Get-Process parcour -ErrorAction SilentlyContinue |
    ForEach-Object { Stop-Process -Id $_.Id -Force }
Start-Sleep -Milliseconds 500
# Now build
```

### 14.5 Inspecting the Tile Map

To verify what the collision system "sees" at a specific pixel position:

```c
int col = px / TILE_SIZE;
int row = py / TILE_SIZE;
game_log("Tile at (%d,%d) = col %d row %d solid=%d wall=%d",
         px, py, col, row, tile_solid(px, py), tile_wall(px, py));
```

---

## 15. Key Design Decisions

### 15.1 Center-Foot-Only Landing

**Problem**: Using left and right foot probes allowed the character to "stand" on a ledge edge with only one foot on solid ground — unrealistic and caused oscillation bugs.

**Decision**: Only the center foot (x + RENDER_W/2) is checked for landing. If the character's center of mass is past the edge, they fall.

**Trade-off**: Slightly less forgiving — the player must land more precisely. But it prevents the entire class of edge-standing bugs.

### 15.2 Wall vs. Thin Platform Distinction

**Problem**: The collision system treated all solid tiles identically. A 1-tile-thick floor platform would block horizontal movement and trigger wall-slide, preventing the character from jumping to platforms on the right side of the level.

**Decision**: Introduced `tile_wall()` — a tile is a "wall" only if it has a solid neighbor above or below (≥2 tiles tall). Horizontal collision and wall-slide use `tile_wall`; landing uses `tile_solid`.

**Trade-off**: A single isolated solid tile (no neighbors in any direction) will be treated as a thin platform. This is acceptable for the current level design.

### 15.3 Full-Body Horizontal Collision with Overlap-Aware Filtering

**Problem**: Earlier iterations either (a) used head probes and got stuck under overhead platforms, or (b) omitted head probes and allowed walking through head-height walls.

**Decision**: Check all 5 body heights (head, upper, mid, lower, foot) but add an **overlap-aware filter**: if the character's CURRENT position is already solid at a given height, don't block horizontal movement at that height. Only block when entering a NEW solid tile.

**Trade-off**: Slightly more complex logic, but solves both problems: the character can walk under ceiling platforms (already overlapping) while still being blocked by new walls at any body height. This went through 6 iterations (see §17.7) before arriving at the current design.

### 15.4 Tiered Barrier Interaction

**Problem**: All horizontal collisions resulted in a hard stop, making single-tile barriers (stairs) feel clunky and requiring tedious jumping.

**Decision**: Three tiers of barrier response based on which body probes are blocked:
- **Foot-only** (1-tile barrier): Auto step-up with 20-frame cooldown
- **Mid/lower** (2-3 tile barrier): Hard stop, manual jump required
- **Head/upper** (tall wall): Pull-up via corner-grab if ledge is nearby

**Trade-off**: The auto-step-up is instant (no animation), which looks slightly unnatural. A future enhancement could add a step-up animation state.

### 15.5 Grab Cooldown

**Problem**: After falling off a ledge or releasing a grab, the character would immediately re-grab the same ledge.

**Decision**: A `grabCooldown` counter (15-20 frames) prevents re-grabbing after releasing or falling off. This creates the natural feel of committing to a fall.

### 15.6 Auto-Climb with Delay

**Problem**: When the head-ledge grab was combined with `inputDir == facing` for auto-climb, the grab animation was skipped (climb triggered on the very next frame because the player was still holding the arrow key).

**Decision**: Auto-climb requires holding forward for 15 frames (~0.5s) in `CORNER_GRAB` state. This shows the full grab animation before climbing. Pressing Up still climbs instantly for responsive gameplay.

---

## 16. Known Limitations and Future Work

### Current Limitations

| Limitation | Impact | Potential Fix |
|------------|--------|---------------|
| No scrolling/camera | Level size = screen size | Camera module with smooth follow |
| Software rendering | CPU-bound at high resolutions | Direct3D or OpenGL backend |
| No sound | Silent gameplay | `PlaySound` or XAudio2 |
| No enemies or hazards | Pure mechanics demo | Enemy AI module |
| Fixed tile types (solid/air) | Limited level design | Tile type enum (spike, ladder, door, etc.) |
| No gamepad support | Keyboard only | XInput integration |
| Auto-step-up has no animation | Instant teleport looks unnatural | Step-up animation state |

### Potential Enhancements

- **Scroll/Camera**: Follow the player across a level larger than the screen
- **Tile variety**: Spikes (instant death), ladders (climb), doors (transition), crumbling floors
- **Enemy AI**: Patrolling guards with simple state machines
- **Health system**: HP + damage + death/respawn
- **Combat**: Sword attacks using the available `adventurer-run-punch` sprites
- **Sound effects**: Jump, land, grab, slide audio cues
- **Particle effects**: Dust on landing, sparks on wall-slide
- **Step-up animation**: Visual feedback for auto-step-up instead of instant teleport

---

## 17. Development Journey — Lessons Learned

This section is a retrospective on how the project evolved, the dead ends we hit, what we learned from each, and how studying Jordan Mechner's original Prince of Persia source code changed the direction entirely.

### 17.1 Starting Point: A Static Array Character

The project began as a single-file `game.c` with a character drawn from a hard-coded pixel array — a 24×32 grid where each character in a string literal mapped to a color:

```
'H' = skin, '#' = shirt, 'P' = pants, 'B' = boots, '.' = transparent
```

A `draw_sprite()` function iterated over this array, painting colored rectangles at `SPRITE_SCALE × SPRITE_SCALE` size into a framebuffer. The character was a blocky stick figure, and "animation" meant swapping between two or three of these arrays.

**What worked:** It got something on screen fast. The build-run-iterate cycle was nearly instant, and it forced us to understand framebuffer rendering, `StretchDIBits`, and the Win32 game loop from the ground up.

**What didn't work:** The character looked crude — there was no way to make it look fluid or human. Designing new poses meant hand-editing 24×32 grids of characters, which was tedious and error-prone. Most importantly, the movement felt like Mario or Dangerous Dave — responsive and snappy, but nothing like Prince of Persia's weight and momentum.

**Lesson:** *Static sprite arrays are great for prototyping but terrible for realistic animation. You need many frames per action, and hand-crafting each frame as a character grid doesn't scale.*

### 17.2 The Physics Overhaul: From Arcade to Realistic

The user correctly identified that the initial movement felt "like Mario, not like PoP." The original code used instant-velocity movement — press right, character moves at full speed; release, character stops dead.

We replaced this with an acceleration-and-friction model:
- **Ground acceleration** builds speed gradually (0 → max over ~10 frames)
- **Multiplicative friction** (0.82× per frame) creates natural deceleration
- **Air control** is severely limited (0.08 vs 0.38 ground accel) — once airborne, you're committed
- **Pre-jump crouch** adds a 3-frame windup before launch

This single change transformed the feel. The character had *weight*. Turning around required deceleration, a brief pause, then re-acceleration. Jumps had commitment — you couldn't change direction mid-air.

**Lesson:** *The difference between arcade and realistic platforming is not in the sprites — it's in the physics model. Acceleration, friction, and air control constraints create the feeling of a physical body with mass and momentum.*

### 17.3 The Skeletal Animation Experiment (and Why It Failed)

Wanting better visuals without external assets, we built a skeletal animation system from scratch:
- 14 joints (head, neck, shoulders, elbows, hands, hips, knees, feet, toes)
- Keyframe poses defined as joint positions relative to the hip
- Smoothstep interpolation between keyframes (t = 3t² − 2t³)
- Limbs drawn as circle-stamped thick lines with outline+fill shading
- Body parts layered in z-order (far arm → far leg → torso → near leg → near arm)

It was an impressive engineering exercise. The character had articulated limbs, proper joint hierarchy, and smooth interpolation. We even added an ellipse-based head with hair, face, and eye details.

**What we learned from it:** Skeletal systems work well in engines with proper rigging tools (Spine, DragonBones). Hand-coding joint positions in C arrays, without visual feedback, is like sculpting blindfolded. Every pose required guessing coordinates, building, running, and tweaking — an incredibly slow feedback loop. The result never looked "right" because the proportions and timing were off in subtle ways that only a trained animator would catch.

**What killed it:** The user's verdict was direct — "this version also sucks." And they were right. No amount of smoothstep interpolation could compensate for amateur-designed poses.

**Lesson:** *Animation is an art discipline, not an engineering problem. A technically sophisticated animation system with bad poses looks worse than simple sprites with good poses. Use artist-created assets whenever possible.*

### 17.4 Studying Mechner's Original Source Code

A turning point came when we studied the actual Prince of Persia Apple II source code (Jordan Mechner's 6502 assembly, released on GitHub for historical study). Two files were revelatory:

**SEQTABLE.S** — The sequence table defined every animation as a list of frames with per-frame displacement commands (`chx`, `chy`). The run cycle wasn't physics-driven at all — each frame specified *exactly* how many pixels the character moves:

```
runcyc1: chx 5    (frame 7: move 5px forward)
runcyc2: chx 1    (frame 8: move 1px)
runcyc3: chx 2    (frame 9: move 2px)
...
```

**FRAMEDEF.S** — Each sprite frame had collision metadata (`Fcheck` flags) baked right into the frame definition. The animation system *was* the physics system.

This was Mechner's key insight: he used **rotoscoped film** of his brother running, jumping, and climbing. Each frame was hand-traced from real human motion. The displacement data came from measuring how far the real person moved between frames. The result was movement that *felt* real because it *was* real — captured from life.

**Lesson:** *PoP's legendary fluidity isn't from clever code — it's from rotoscoped human motion. The animation drives the movement, not the other way around. This is fundamentally different from physics-driven engines where velocity × time = displacement.*

### 17.5 The Pivot to Pre-Rendered Sprite Sheets

Armed with this understanding, we abandoned the skeletal system entirely and switched to the rvros "Animated Pixel Adventurer" sprite pack — a free, professionally animated sprite sheet with 39 animations and dozens of frames per action.

The difference was immediate and dramatic. With proper artist-drawn sprites:
- **Idle** had a subtle breathing animation across 4 frames
- **Run** had a full 6-frame cycle with proper contact-recoil-push gait
- **Jump** had separate rise and fall animations
- **Corner grab and climb** looked natural and convincing
- **Wall slide** had cloth physics and wind effects baked into the art

We kept our physics-driven state machine but now used it to *select* which animation to play rather than to *generate* the animation. The state machine became a puppeteer choosing sprite sequences, while the artist's work provided the visual quality we could never achieve procedurally.

**Lesson:** *The right architecture is a state machine driving sprite selection, not sprite generation. Separate the "what should happen" (state machine + physics) from the "what does it look like" (artist-created sprites).*

### 17.6 Making Gravity Realistic

Early gravity was trivially simple: `vy += GRAVITY` each frame, with a terminal velocity cap. This meant falling from 1 tile and falling from 10 tiles felt nearly identical — the character reached terminal velocity almost instantly.

We modularized gravity into its own `gravity.c` module and implemented a proper model:
- **Quadratic air drag**: `vy += GRAV_ACCEL - GRAV_DRAG × vy²`
- **Asymptotic terminal velocity**: speed approaches but never exceeds √(GRAV_ACCEL / GRAV_DRAG)
- **Landing tier classification**: fall distance determines recovery animation
  - Short falls (< 4 tiles): normal landing
  - Medium falls (4-8 tiles): hard landing with roll (somersault)
  - Long falls (> 8 tiles): would be death in a full game

The drag model means the character accelerates quickly at first but the acceleration diminishes as speed increases — exactly like real-world terminal velocity. A short fall is noticeably slower than a long fall, and the landing animation reflects the impact force.

**Lesson:** *Real gravity isn't linear. Air drag creates the "weight" that makes falls feel dangerous. And classifying landing severity by fall height lets you create meaningful consequences for risky jumps — the soul of PoP's level design.*

### 17.7 The Collision Refinement Journey

Collision detection went through several iterations, each fixing bugs introduced by the previous approach:

1. **Naive AABB**: Check all four corners of the bounding box against tiles. Problem: character could "stand" on ledges with one pixel of overlap, causing oscillation between standing and falling states.

2. **Center-foot rule**: Only the center of the character's foot (x + RENDER_W/2) determines landing. If the center-of-mass is past the edge, you fall. This eliminated edge-standing but felt strict.

3. **Wall vs. platform distinction**: All solid tiles were treated equally — a single-row floor platform would block horizontal movement and trigger wall-slide. We introduced `tile_wall()`: a tile is a "wall" only if it has a vertically adjacent solid tile (≥2 tiles tall). Horizontal collision uses `tile_wall`; landing uses `tile_solid`. This let the character walk through thin platforms from the side while still landing on them from above.

4. **No head probes in horizontal collision**: Head-level probes caused the character to get stuck under overhead platforms. Removing them from horizontal checks and handling head-height platforms through `tile_head_ledge()` in the state machine eliminated an entire class of stuck-state bugs.

5. **Grab cooldown**: After releasing or falling off a ledge, a 15-frame cooldown prevents immediate re-grabbing, which caused oscillation loops (grab → release → fall → grab → ...).

6. **Full-body collision with overlap-aware filtering**: Re-added head probes (all 5 body heights) to prevent walking through head-height walls, but added an overlap-aware filter: if the character's current position is already solid at a probe height, don't block movement at that height. This solves the "stuck under ceiling" bug without removing head collision. Horizontal collision now uses `tile_solid()` at all heights instead of `tile_wall()` at mid+lower. Combined with leading-edge-only checks and the barrier interaction tiers (auto-step-up for foot-level, hard stop for chest-level, pull-up for head-level), this is the final iteration.

**Lesson:** *Collision systems evolve by fixing the bugs they create. Each refinement solves one problem and reveals the next. The trick is to fix at the right level of abstraction — collision probes for physics bugs, state machine logic for behavioral bugs, cooldown timers for oscillation bugs, and overlap-aware filters for legitimately overlapping geometry.*

### 17.8 The Power of Logging

The single most valuable debugging tool throughout the project was file-based logging. Every state transition, collision event, and physics decision was logged with sub-millisecond timestamps to `parcour.log`.

Bugs that would have taken hours to reproduce and debug visually were diagnosed in minutes by reading the log:
- **Oscillation**: Spotted as `IDLE → FALL → IDLE → FALL → IDLE` repeating at the same position every frame
- **Getting stuck**: No state transitions despite input being logged as active
- **False triggers**: Unexpected `CORNER_GRAB` at positions where no ledge existed — probe coordinates in the log revealed an off-by-one in the tile lookup

The log was especially critical because the game runs at 60fps — bugs that manifest for a single frame are invisible visually but clearly recorded in the log.

**Lesson:** *In a game running at 60fps, your eyes can't debug physics. Log everything — state transitions, positions, velocities, probe results. When something goes wrong, `grep` the log for the anomaly. This is how real game engines are debugged.*

### 17.9 Summary of Key Realizations

| # | Realization | Impact |
|---|-------------|--------|
| 1 | Realistic movement comes from physics (acceleration + friction), not from sprite art | Changed the entire feel from arcade to cinematic |
| 2 | Animation is an art discipline — use artist-created assets | Abandoned skeletal system, adopted sprite sheets |
| 3 | Mechner's rotoscoping was the secret to PoP's fluidity | Validated that we were right to use pre-animated sprites |
| 4 | State machines should drive sprite *selection*, not sprite *generation* | Clean architecture separating logic from presentation |
| 5 | Gravity needs air drag for realism | Falls feel weighted and dangerous instead of uniform |
| 6 | Collision refinement is iterative — each fix reveals the next bug | Evolved through 6 iterations of collision logic |
| 7 | Logging is the most powerful debugging tool for real-time systems | Found and fixed every physics bug via log analysis |
| 8 | Modular architecture enables testability | 81 automated tests catch regressions on every build |
| 9 | Overlap-aware filtering solves the "stuck under ceilings" class of bugs | Characters can legitimately overlap solid tiles they're already inside |
| 10 | Tiered barrier interaction creates natural PoP-style movement flow | Small barriers stepped, medium barriers jumped, tall barriers pulled-up |

---

## Appendix A: Constants Reference

### Display

| Constant | Value | Defined In |
|----------|------:|------------|
| `SCREEN_W` | 1280 | types.h |
| `SCREEN_H` | 960 | types.h |
| `TILE_SIZE` | 32 | types.h |
| `LEVEL_COLS` | 40 | types.h |
| `LEVEL_ROWS` | 30 | types.h |
| `RENDER_W` | 64 | types.h |
| `RENDER_H` | 128 | types.h |
| `FPS` | 60 | types.h |
| `SPRITE_DRAW_SCALE` | 3 | character.h |

### Physics

| Constant | Value | Defined In |
|----------|------:|------------|
| `GROUND_ACCEL` | 0.55 | physics.h |
| `GROUND_FRICTION` | 0.82 | physics.h |
| `AIR_CONTROL` | 0.08 | physics.h |
| `AIR_FRICTION` | 0.98 | physics.h |
| `MAX_RUN_SPEED` | 6.0 | physics.h |
| `STOP_THRESHOLD` | 0.05 | physics.h |
| `JUMP_IMPULSE` | -10.0 | physics.h |
| `JUMP_HOLD_BOOST` | 0.35 | physics.h |
| `JUMP_HOLD_MAX` | 10 | physics.h |

### Gravity

| Constant | Value | Defined In |
|----------|------:|------------|
| `GRAV_ACCEL` | 0.48 | gravity.h |
| `GRAV_TERMINAL` | 18.0 | gravity.h |
| `GRAV_DRAG` | 0.015 | gravity.h |
| `LANDING_SOFT_VY` | 4.0 | gravity.h |
| `LANDING_MED_VY` | 8.0 | gravity.h |

### Timing

| Constant | Value | Defined In |
|----------|------:|------------|
| `LANDING_TICKS` | 8 | physics.h |
| `TURN_TICKS` | 5 | physics.h |
| `STEP_COOLDOWN` | 20 | character.c |
| `FRAME_MS` | 16 | types.h |

---

## Appendix B: Level Layout

```
        0         1         2         3
Col:    0123456789012345678901234567890123456789
Row 0:  WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW  ← ceiling
Row 1:  W......................................W
Row 2:  W......................................W
Row 3:  WWWWWW.........................PPPPP..W  ← tower top + right high
Row 4:  WW.....................................W
Row 5:  WW.....................................W
Row 6:  WW.....................................W
Row 7:  WW.........................PPPPPPPPP..W  ← right mid-high
Row 8:  WW.....................................W
Row 9:  WW.....................................W
Row10:  WW.............PPPPPP..................W  ← center high
Row11:  WW.........................PPPPP......W  ← right mid
Row12:  WW.....................................W
Row13:  WW.....................................W
Row14:  WW........PPPPP........................W  ← center mid
Row15:  WW.........................PPPPP......W  ← right mid-low
Row16:  WW.....................................W
Row17:  WW.....................................W
Row18:  WW..................PPPPPP..............W  ← center low
Row19:  WW.........................PPPPP......W  ← right low
Row20:  WW.....................................W
Row21:  WW.....................................W
Row22:  WW...........PPPPP....................W  ← lower center
Row23:  WW.........................PPPPP......W  ← right ground-level
Row24:  WW.....................................W
Row25:  WW.....................................W
Row26:  WWWWWWWW.......................WWWWWWWWW  ← ground platforms
Row27:  W......................................W
Row28:  W......................................W
Row29:  WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW  ← floor

Legend: W = wall (multi-tile solid)  P = thin platform  . = air
```

---

## Appendix C: Sprite Asset Inventory

**Source**: rvros "Animated Pixel Adventurer" (itch.io, free)  
**Format**: Individual PNG files, 50×37 pixels, RGBA with transparency  
**Display**: Rendered at 3× scale = 150×111 pixels on screen  

| Animation | File Prefix | Frames | Used By State |
|-----------|-------------|-------:|---------------|
| Idle | `adventurer-idle` | 4 | `STATE_IDLE` |
| Walk | `adventurer-walk` | 6 | `STATE_WALK` |
| Run | `adventurer-run` | 6 | `STATE_RUNNING` |
| Stand (stop/turn) | `adventurer-stand` | 3 | `STATE_STOPPING`, `STATE_TURNING` |
| Crouch | `adventurer-crouch` | 4 | `STATE_CROUCH` |
| Jump (rise) | `adventurer-jump` | 4 | `STATE_JUMP_RISE` |
| Fall | `adventurer-fall` | 2 | `STATE_JUMP_FALL` |
| Somersault | `adventurer-smrslt` | 4 | `STATE_LANDING`, `STATE_SOMERSAULT`, `STATE_HARD_LANDING` |
| Corner Grab | `adventurer-crnr-grb` | 4 | `STATE_CORNER_GRAB` |
| Corner Climb | `adventurer-crnr-clmb` | 5 | `STATE_CORNER_CLIMB` |
| Wall Slide | `adventurer-wall-slide` | 2 | `STATE_WALL_SLIDE` |
| Slide | `adventurer-slide` | 4 | `STATE_SLIDE` |

**Unused but available**: `adventurer-crouch-walk` (6), `adventurer-run-punch` (7), `adventurer-idle-2` (4), `adventurer-bow` (Aseprite source)

---

*End of Design Document*
