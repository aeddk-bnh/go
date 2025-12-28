# Technical Overview

This document describes architecture, data structures, and technical requirements for the Go project.

## Architecture
- `Board` — board representation, group detection, capture, liberty checks.
- `Game` — manages turns, move history, rule enforcement, scoring.
- `Rules` — pluggable rulesets (Japanese, Chinese, Tromp-Taylor) and scoring strategies.
- `AI` — engines (MCTS) live in `src/ai/`.
- `UI` — `console/` and optional `gui/` (Qt/SFML) frontends.
- `IO` — SGF parsing and writing.
- `Network` — multiplayer layer (Boost.Asio recommended).

## Data structures & algorithms
- Board: 1D vector of ints (size N*N) or bitboard for optimized variants.
- Capture detection: flood-fill / DFS with union-find optional optimizations.
- Superko detection: Zobrist hashing for fast repetition detection.
- AI: Monte Carlo Tree Search with UCT. Use transposition tables and virtual loss for multi-threading.

## Performance notes
- Use Zobrist hashes to avoid expensive board comparisons.
- Keep hot loops cache-friendly; use 1D arrays to improve locality.

## Build & Tooling
- Use CMake for cross-platform builds.
- Unit tests using GoogleTest.
- Format code with `clang-format` and enforce on CI.


> Keep this file updated when architecture or critical algorithms change.