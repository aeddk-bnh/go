**Go Console — Tutorial: New Modes**

This document explains the non-interactive and AI modes added to the console program (`go_console`). It covers interactive commands, the autorun script mode, and the headless batch self-play mode (CSV output).

1) Interactive commands (console)
- **`mcts [iters]`**: run MCTS for the given number of iterations and play the resulting move.
- **`mctst [sec]`**: run time-limited MCTS for ~`sec` seconds and play the selected move.
- **`ai`**: make the AI play one move (uses default timing/threads configured in program).
- **`playai [B|W|both] [secs] [threads] [Cp]`**: enable automatic play by AI. Example: `playai both 2 4 1.414` runs AI on both sides, 2s per move, 4 threads, Cp=1.414.
- **`stopai`**: stop play-with-AI mode.
- **`pass` / `undo` / `score` / `quit`**: standard controls.

2) Autorun (scripted input)
- Use the `GO_AUTORUN` environment variable to point the program at a plain-text script file containing lines to feed the console as stdin. When present, the program reads the file's lines and acts as if a user typed them.
- Typical usage (PowerShell):

```powershell
$env:GO_AUTORUN = 'D:\path\to\script.txt'
.\build\src\Release\go_console.exe
```

The script file can contain console commands (see section 1) and menu answers (e.g., board size at startup).

3) Batch self-play (headless) — generate CSV
- The program supports a headless batch mode controlled entirely by environment variables. When `GO_BATCH_COUNT` is set (>=1) the program runs that many self-play games and exits.
- Important environment variables:
  - `GO_BATCH_COUNT` — number of self-play games to run (int)
  - `GO_BATCH_SECS` — seconds per move decision (float)
  - `GO_BATCH_THREADS` — number of worker threads for parallel MCTS (int)
  - `GO_BATCH_CP` — MCTS Cp exploration constant (float)
  - `GO_BATCH_BOARD` — board size N (int, default 9)
  - `GO_BATCH_OUT` — output CSV path (default `D:/go/automation/sim_results.csv`)

Example (PowerShell) — run 3 games, 0.5s per move, 2 threads, write clean CSV:

```powershell
# remove any previous output then run clean batch
$out = 'D:\go\automation\sim_results_clean.csv'
if(Test-Path $out){ Remove-Item $out -Force }
$env:GO_BATCH_COUNT='3'
$env:GO_BATCH_SECS='0.5'
$env:GO_BATCH_THREADS='2'
$env:GO_BATCH_CP='1.4'
$env:GO_BATCH_BOARD='9'
$env:GO_BATCH_OUT=$out
.\build\src\Release\go_console.exe > D:\go\automation\batch_clean_log.txt 2>&1
# results at: D:\go\automation\sim_results_clean.csv
```

Notes:
- The batch mode writes a CSV with header `game_id,winner,black_total,white_total,moves,duration_s`.
- The repository now ignores `automation/` outputs by default (see `.gitignore`), so results/logs won't be committed.
- If you want to append runs into the same CSV, set `GO_BATCH_OUT` to an existing file; the program will append rows but only write the header when the file is empty.

4) Background thinker
- The background thinker is used by `playai` to keep improving the root MCTS tree between moves. It is internal and generally does not require user configuration, though `playai` accepts `threads` and `secs` parameters that influence behavior.

5) Troubleshooting
- If you see crashes in batch/multi-threaded runs, try lowering `GO_BATCH_THREADS` to 1 and increasing `GO_BATCH_SECS` to give the single-threaded MCTS more time. Report logs from `batch_clean_log.txt` if problems persist.

6) Where to find things
- Source: `src/main.cpp` (batch, autorun, playai handlers)
- Logs generated during runs: `D:\go\automation\batch_clean_log.txt` (example)
- CSV results: `D:\go\automation\sim_results_clean.csv` (example)

If you'd like, I can add a short note to `README.md` linking to this file and give some recommended default parameters for larger experiments.
