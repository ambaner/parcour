# Contributing to Parcour

Thanks for your interest in contributing! This is a learning project, and contributions of all kinds are welcome.

## How to Contribute

### Reporting Bugs

1. Open an [Issue](../../issues/new?template=bug_report.md) using the bug report template.
2. Include steps to reproduce, expected vs. actual behavior, and your build configuration.
3. If the bug involves a specific game state, describe the character position, state, and inputs.

### Suggesting Features

1. Open an [Issue](../../issues/new?template=feature_request.md) using the feature request template.
2. Describe the feature and why it would be valuable.
3. If it relates to Prince of Persia mechanics, link to reference material if possible.

### Submitting Code Changes

1. **Fork** the repository.
2. **Create a branch** from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes** following the coding guidelines below.
4. **Build and test**:
   ```cmd
   build.bat fre
   ```
   All 52+ tests must pass before submitting.
5. **Commit** with a clear, descriptive message:
   ```
   Add wall-jump mechanic with velocity boost
   ```
6. **Push** and open a **Pull Request** against `main`.

## Coding Guidelines

### Language & Style

- **Pure C** (C17) — no C++ features, no `//` comments in headers
- **Win32 API** only — no external frameworks or game engines
- **4-space indentation**, no tabs
- **`snake_case`** for functions and variables
- **`SCREAMING_CASE`** for constants and macros
- **`PascalCase`** for type names (structs, enums)

### File Organization

- **Engine code** (`src/engine/`) — generic, reusable, no game-specific knowledge
- **Game code** (`src/game/`) — game-specific logic, character, levels
- **Tests** (`test/engine/` and `test/game/`) — one test file per module

### Every Source File Must Have

1. A descriptive comment header explaining the module's purpose
2. Include guards for `.h` files (`#ifndef` / `#define` / `#endif`)

### Adding New Files

1. Add source files to the appropriate `src/engine/` or `src/game/` directory.
2. Update `build.bat` to include the new file in the compile command.
3. Add corresponding tests in `test/engine/` or `test/game/`.
4. Register the test suite in `test/test_main.c`.
5. Update `doc/DESIGN.md` module reference (Section 5) and directory tree (Section 4).

### Tests

- Every new feature or bug fix should include tests.
- Use the macros in `test/test_framework.h`: `TEST()`, `ASSERT_TRUE()`, `ASSERT_EQ()`, etc.
- Tests must be **headless** — no window creation, no rendering, no user input.
- Run `build.bat fre` to verify all tests pass.

## Architecture Overview

```
engine.lib (static)  ←  Generic 2D engine (math, gravity, physics, input, renderer, log)
     ↓
parcour.exe           ←  Game code (character FSM, levels, sprites) + engine.lib + sprites.res
     
test_runner.exe      ←  Test code + game modules + engine.lib (no sprites.res)
```

See [doc/DESIGN.md](doc/DESIGN.md) for the full architecture documentation.

## What Makes a Good Pull Request

- **Focused** — one feature or fix per PR
- **Tested** — includes new or updated tests
- **Documented** — updates DESIGN.md if architecture changes
- **Builds clean** — `build.bat fre` with `/W4 /WX` (zero warnings)
- **Backward compatible** — doesn't break existing controls or physics feel

## Questions?

Open a [Discussion](../../discussions) or file an Issue. Happy hacking! 🎮
