Write-Host "Locating g++.exe on the system (PATH or common installs)..."

# Try Get-Command first
$gppCmd = Get-Command g++ -ErrorAction SilentlyContinue
$gppPath = $null
if ($gppCmd) { $gppPath = $gppCmd.Source }

if (-not $gppPath) {
    # Search common locations for g++.exe
    $searchRoots = @('C:\msys64','C:\MinGW','C:\TDM-GCC','C:\Program Files','C:\Program Files (x86)')
    foreach ($r in $searchRoots) {
        if (Test-Path $r) {
            $found = Get-ChildItem -Path $r -Filter g++.exe -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($found) { $gppPath = $found.FullName; break }
        }
    }
}

if (-not $gppPath) {
    Write-Host "No g++.exe found on the system. Please install MinGW/msys2 or place a winlibs zip at tools/winlibs.zip and run install_mingw.ps1"
    exit 1
}

Write-Host "Found g++ at: $gppPath"

$dest = Join-Path (Get-Location) "tools\mingw-w64"
if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
New-Item -ItemType Directory -Path $dest | Out-Null

# Decide what to copy based on location of g++
$gppDir = Split-Path $gppPath -Parent
if ($gppPath -match "\\usr\\bin\\g\+\+\.exe$") {
    # msys2 style: root is two levels up (msys64)
    $msysRoot = Split-Path $gppDir -Parent
    Write-Host "Detected MSYS2: copying $msysRoot\usr and $msysRoot\mingw64 (if present)"
    try {
        Copy-Item -Path (Join-Path $msysRoot 'usr') -Destination (Join-Path $dest 'usr') -Recurse -Force -ErrorAction Stop
    } catch { Write-Host "Warning: failed to copy usr: $_" }
    if (Test-Path (Join-Path $msysRoot 'mingw64')) {
        try { Copy-Item -Path (Join-Path $msysRoot 'mingw64') -Destination (Join-Path $dest 'mingw64') -Recurse -Force -ErrorAction Stop } catch { Write-Host "Warning: failed to copy mingw64: $_" }
    }
} elseif ($gppPath -match "\\MinGW\\bin\\g\+\+\.exe$" -or $gppPath -match "\\TDM-GCC\\bin\\g\+\+\.exe$") {
    # Classic MinGW: copy the whole MinGW root
    $root = Split-Path $gppDir -Parent
    Write-Host "Detected MinGW: copying $root"
    try { Copy-Item -Path $root\* -Destination $dest -Recurse -Force -ErrorAction Stop } catch { Write-Host "Warning: failed to copy MinGW root: $_" }
} else {
    # Fallback: copy the bin folder containing g++ and sibling lib folders if present
    Write-Host "Copying bin directory: $gppDir"
    try { Copy-Item -Path $gppDir -Destination (Join-Path $dest 'bin') -Recurse -Force -ErrorAction Stop } catch { Write-Host "Warning: failed to copy bin: $_" }
    $parent = Split-Path $gppDir -Parent
    if (Test-Path (Join-Path $parent 'lib')) { try { Copy-Item -Path (Join-Path $parent 'lib') -Destination (Join-Path $dest 'lib') -Recurse -Force -ErrorAction Stop } catch {} }
}

$gpp = Get-ChildItem -Path $dest -Recurse -Filter g++.exe -ErrorAction SilentlyContinue | Select-Object -First 1
if ($gpp) {
    Write-Host "Restored MinGW. Found g++ at: $($gpp.FullName)"
    Write-Host "You can now rerun CMake, e.g."
    Write-Host "cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake -DCMAKE_MAKE_PROGRAM=\"$env:USERPROFILE\\.conan2\\p\\ninja*/p/bin/ninja.exe\" -DCMAKE_CXX_COMPILER=\"$($gpp.FullName)\" -DCMAKE_BUILD_TYPE=Release"
    exit 0
} else {
    Write-Host "Copy completed but g++.exe not found under $dest"
    exit 3
}
