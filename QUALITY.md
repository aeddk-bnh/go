# Quality Control & Testing

This file describes the project's quality control requirements and processes.

## Testing
- Unit tests: cover core game logic (capture, ko, scoring) and boundary cases.
- Integration tests: full-game simulations, SGF read/write validation.
- Test coverage: aim for high coverage on core modules (Board, Game, Rules).

## Continuous Integration
CI should run on push and PRs:
- Configure the CI to build in Debug and Release modes.
- Run unit tests with `ctest`.
- Run static analyzers and linters (e.g., `clang-tidy`, `cpplint`).
- Run `clang-format` in check mode to fail on formatting drift.

## Linters and formatters
- `clang-format` for code style. Add `.clang-format` at repo root.
- `clang-tidy` for static checks (configurable via `.clang-tidy`).
- Optionally `cpplint` for additional style checks.

## Code Review & Merge Policy
- Passing CI required before merging.
- At least one approving review from a project maintainer.
- Use descriptive commit messages and PR descriptions.

## Release QA
- Tag release, run full test matrix, create release notes.

## Running checks locally
- Format check: `clang-format -style=file -i <files>` or to check without changing: `clang-format -style=file --dry-run --Werror <files>` (or use `git-clang-format`).
- Static analysis: `clang-tidy` run via CMake build or directly: `clang-tidy <file> -- -Iinclude`.
- cpplint: use `CPPLINT.cfg` and run `cpplint --recursive src include`.

> Add automation scripts and sample CI configs to the `.github/workflows` directory.