# PR Checklist

Before opening a Pull Request, ensure the following:

- [ ] Branch name follows `feature/` or `fix/` pattern.
- [ ] All new code has unit tests and tests pass locally.
- [ ] `clang-format` has been applied and formatting is consistent: `clang-format -style=file -i`.
- [ ] Static analysis (clang-tidy) run and addressed as needed.
- [ ] CI runs clean on your branch (push and check GitHub Actions).
- [ ] Update `CHANGELOG.md` or add release notes if behaviour changes.
- [ ] Add PR description explaining rationale, testing done, and any migration notes.

Suggested commands:
```bash
# create a branch and commit
git checkout -b feature/my-changes
git add .
git commit -m "feat: add docs and CI scaffolding"
git push -u origin feature/my-changes
# open PR on GitHub targeting main
```

Reviewer checklist:
- Is the change small and focused?
- Are tests sufficient and deterministic?
- Are there any performance regressions?
- Does the code follow the guidelines in `TECHNICAL.md` and `CONTRIBUTING.md`?
