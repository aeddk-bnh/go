# Go Game (C++)

A C++ implementation of the board game Go. This repository contains the core engine, a console UI, and scaffolding for GUI, AI, and network play.

## Quick start

Requirements: C++17, CMake, a C++ compiler (gcc/clang/Visual Studio).

```bash
# build (out-of-source)
mkdir build && cd build
cmake ..
cmake --build . --config Release

# run tests
ctest --output-on-failure
```

## Structure
- `src/` — core sources (Board, Game, Rules)
- `tests/` — unit tests (GoogleTest)
- `docs/` — design and process docs

## Development
- Follow C++ style in `TECHNICAL.md` and `QUALITY.md`.
- CI will run build, tests, and linters on PRs.

## Developer setup
1. Install prerequisites: CMake, a C++17-capable compiler, and optional tools: `clang-format`, `clang-tidy`.
2. Build:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```
3. Run tests:
```bash
ctest --output-on-failure
```
4. Run formatting and lint checks:
```bash
clang-format -style=file -i $(git ls-files '*.cpp' '*.hpp' '*.h')
clang-tidy <file> -- -Iinclude
```

---

For contribution guidelines, see `CONTRIBUTING.md`.