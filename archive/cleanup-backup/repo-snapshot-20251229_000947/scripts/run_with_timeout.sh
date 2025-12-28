#!/usr/bin/env bash
# Usage: run_with_timeout.sh <seconds> <command...>
# Runs command with timeout (uses coreutils timeout)
if [ $# -lt 2 ]; then
  echo "Usage: $0 <seconds> <command...>" >&2
  exit 2
fi
T=$1; shift
cmd="$@"
# Use timeout if available
if command -v timeout >/dev/null 2>&1; then
  timeout ${T}s $cmd
  exit_code=$?
  if [ $exit_code -eq 124 ]; then
    echo "Command timed out after ${T}s" >&2
  fi
  exit $exit_code
else
  # Fallback: run in background and kill after timeout
  $cmd &
  pid=$!
  ( sleep $T; kill -9 $pid 2>/dev/null ) &
  watcher=$!
  wait $pid 2>/dev/null
  rc=$?
  kill -9 $watcher 2>/dev/null || true
  if [ $rc -gt 128 ]; then
    echo "Command timed out after ${T}s" >&2
    exit 124
  fi
  exit $rc
fi
