#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./scripts/build.sh [build_dir] [config]
# Flags via env or arguments: set env vars USE_CONAN=0 to skip conan
# Special flags (prefixed after args): --find-compiler --install-mingw --restore-mingw --setup-conan

BUILD_DIR=${1:-build}
CONFIG=${2:-Release}

# parse remaining args for flags
FIND_COMPILER=0
INSTALL_MINGW=0
RESTORE_MINGW=0
SETUP_CONAN_FLAG=0

shift 2 || true
while [ "$#" -gt 0 ]; do
  case "$1" in
    --find-compiler) FIND_COMPILER=1; shift ;;
    --install-mingw) INSTALL_MINGW=1; shift ;;
    --restore-mingw) RESTORE_MINGW=1; shift ;;
    --setup-conan) SETUP_CONAN_FLAG=1; shift ;;
    *) shift ;;
  esac
done

USE_CONAN=${USE_CONAN:-1}

# Add python scripts to PATH for conan (common location)
PY_SCRIPTS="$HOME/.local/bin"
export PATH="$PY_SCRIPTS:$PATH"

echo "Build dir: $BUILD_DIR  Config: $CONFIG  UseConan: $USE_CONAN"

# Helper: find compilers
find_compiler() {
  candidates=()
  if command -v g++ >/dev/null 2>&1; then candidates+=("$(command -v g++)"); fi
  if command -v clang++ >/dev/null 2>&1; then candidates+=("$(command -v clang++)"); fi
  if command -v cl >/dev/null 2>&1; then candidates+=("$(command -v cl)"); fi
  # known locations
  for k in "/c/msys64/mingw64/bin/g++" "/c/msys64/usr/bin/g++" "/c/MinGW/bin/g++" "/c/TDM-GCC/bin/g++" "/usr/bin/g++"; do
    [ -x "$k" ] && candidates+=("$k")
  done
  # unique
  printf "%s\n" "${candidates[@]}" | awk '!x[$0]++'
}

# Helper: install mingw winlibs (download + extract)
install_mingw_from_winlibs() {
  OUTDIR="tools/mingw-w64"
  URLS=(
    "https://github.com/brechtsanders/winlibs_mingw/releases/download/2023-12-20/winlibs-x86_64-posix-seh-gcc-13.2.0-20231220.zip"
    "https://github.com/brechtsanders/winlibs_mingw/releases/download/2024-02-14/winlibs-x86_64-posix-seh-gcc-13.2.0-20240214.zip"
    "https://github.com/brechtsanders/winlibs_mingw/releases/download/2024-12-10/winlibs-x86_64-posix-seh-gcc-14.1.0-20241210.zip"
    "https://winlibs.com/downloads/winlibs-x86_64-posix-seh-gcc-13.2.0-20231220.zip"
  )
  LOCAL_ZIP="tools/winlibs.zip"
  if [ -f "$LOCAL_ZIP" ]; then
    ZIPPATH="$LOCAL_ZIP"
  else
    ZIPPATH="/tmp/winlibs_mingw.zip"
    [ -f "$ZIPPATH" ] && rm -f "$ZIPPATH"
    downloaded=0
    for u in "${URLS[@]}"; do
      echo "Trying: $u"
      if command -v curl >/dev/null 2>&1; then
        if curl -fSL -o "$ZIPPATH" "$u"; then downloaded=1; break; fi
      elif command -v wget >/dev/null 2>&1; then
        if wget -O "$ZIPPATH" "$u"; then downloaded=1; break; fi
      fi
    done
    if [ "$downloaded" -ne 1 ]; then echo "Failed to download winlibs; place archive at tools/winlibs.zip"; return 1; fi
  fi
  EXTRACT_DIR="/tmp/winlibs_extracted"
  rm -rf "$EXTRACT_DIR" && mkdir -p "$EXTRACT_DIR"
  if command -v unzip >/dev/null 2>&1; then
    unzip -q "$ZIPPATH" -d "$EXTRACT_DIR"
  else
    python - <<'PY'
import sys,zipfile
zf=zipfile.ZipFile(sys.argv[1])
zf.extractall(sys.argv[2])
PY "$ZIPPATH" "$EXTRACT_DIR"
  fi
  inner=$(find "$EXTRACT_DIR" -maxdepth 1 -type d | tail -n +2 | head -n1)
  if [ -z "$inner" ]; then echo "No extracted root"; return 2; fi
  rm -rf "$OUTDIR" && mkdir -p "$OUTDIR"
  mv "$inner"/* "$OUTDIR" || true
  rm -f "$ZIPPATH"
  rm -rf "$EXTRACT_DIR"
  if find "$OUTDIR" -name g++.exe -print -quit | grep -q .; then echo "Installed mingw to $OUTDIR"; return 0; fi
  echo "g++.exe not found under $OUTDIR"; return 3
}

# Helper: restore mingw from system paths (copy)
restore_mingw_from_system() {
  DEST="tools/mingw-w64"
  # try which g++ or common locations
  GPP=$(command -v g++ 2>/dev/null || true)
  if [ -z "$GPP" ]; then
    for p in "/c/msys64/usr/bin/g++" "/c/msys64/mingw64/bin/g++" "/c/MinGW/bin/g++" "/usr/bin/g++"; do
      [ -x "$p" ] && { GPP="$p"; break; }
    done
  fi
  if [ -z "$GPP" ]; then echo "No g++ found on system"; return 1; fi
  echo "Found g++ at: $GPP"
  GPPDIR=$(dirname "$GPP")
  rm -rf "$DEST" && mkdir -p "$DEST"
  # Copy parent tree conservatively
  cp -r "$GPPDIR" "$DEST/bin" 2>/dev/null || true
  parent=$(dirname "$GPPDIR")
  if [ -d "$parent/lib" ]; then cp -r "$parent/lib" "$DEST/" 2>/dev/null || true; fi
  if find "$DEST" -name g++ -print -quit | grep -q .; then echo "Restored MinGW into $DEST"; return 0; fi
  echo "Restore completed but g++ not found under $DEST"; return 2
}

# Run optional helpers if requested
if [ "$FIND_COMPILER" -eq 1 ]; then find_compiler; fi
if [ "$INSTALL_MINGW" -eq 1 ]; then install_mingw_from_winlibs || exit 3; fi
if [ "$RESTORE_MINGW" -eq 1 ]; then restore_mingw_from_system || exit 4; fi

# If setup conan flag set, run conan install regardless
if [ "$SETUP_CONAN_FLAG" -eq 1 ]; then
  if command -v conan >/dev/null 2>&1; then
    conan install . --output-folder "$BUILD_DIR" --build missing
  else
    python -m conan install . --output-folder "$BUILD_DIR" --build missing
  fi
fi

# If neither helper flags nor setup-only flag were meant to stop, continue with regular build

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

if [ "$USE_CONAN" -eq 1 ] && [ "$SETUP_CONAN_FLAG" -ne 1 ]; then
  if command -v conan >/dev/null 2>&1; then
    conan install . --output-folder "$BUILD_DIR" --build missing
  else
    python -m conan install . --output-folder "$BUILD_DIR" --build missing
  fi
fi

TOOLCHAIN="$BUILD_DIR/conan_toolchain.cmake"
if [ -f "$TOOLCHAIN" ]; then
  TC_ARG="-DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN"
else
  TC_ARG=""
fi

cmake -S . -B "$BUILD_DIR" -G Ninja $TC_ARG -DCMAKE_BUILD_TYPE="$CONFIG" -DBUILD_BENCHMARKS=OFF -DBUILD_TESTING=ON
cmake --build "$BUILD_DIR" -- -j$(nproc 2>/dev/null || echo 1)

echo "To run tests: ctest --test-dir $BUILD_DIR/tests --output-on-failure"
