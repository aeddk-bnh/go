param(
  [string]$BuildDir = "build",
  [string]$Config = "Release",
  [switch]$UseConan = $true,
  [switch]$SkipDeps = $false,
  [switch]$FindCompiler,
  [switch]$InstallMingw,
  [switch]$RestoreMingw,
  [switch]$SetupConan,
  [switch]$RunTests
)

$ErrorActionPreference = 'Stop'

# Ensure Python scripts dir on PATH (Conan installed via pip)
$pyScripts = Join-Path $env:USERPROFILE "AppData\Roaming\Python\Python313\Scripts"
if (Test-Path $pyScripts) { $env:PATH = "$pyScripts;$env:PATH" }

# --- Embedded helpers (merged from tools/*.ps1) ---
function Find-Compiler {
  $candidates = @()
  try { $g = Get-Command g++ -ErrorAction SilentlyContinue; if ($g) { $candidates += $g.Source } } catch {}
  try { $c = Get-Command clang++ -ErrorAction SilentlyContinue; if ($c) { $candidates += $c.Source } } catch {}
  try { $cl = Get-Command cl -ErrorAction SilentlyContinue; if ($cl) { $candidates += $cl.Source } } catch {}

  $known = @(
    'C:\\msys64\\mingw64\\bin\\g++.exe',
    'C:\\msys64\\usr\\bin\\g++.exe',
    'C:\\MinGW\\bin\\g++.exe',
    'C:\\TDM-GCC\\bin\\g++.exe',
    'C:\\Program Files\\LLVM\\bin\\clang++.exe',
    'C:\\Program Files\\LLVM\\bin\\clang.exe'
  )

  foreach ($k in $known) { if (Test-Path $k) { $candidates += (Get-Item $k).FullName } }
  $candidates = $candidates | Select-Object -Unique
  if ($candidates.Count -eq 0) {
    Write-Host "No compilers found in PATH or common install locations."
    return @()
  }
  return $candidates
}

function Install-MingwFromWinlibs {
  param([string]$OutDir = "tools/mingw-w64")
  $Urls = @(
    'https://github.com/brechtsanders/winlibs_mingw/releases/download/2023-12-20/winlibs-x86_64-posix-seh-gcc-13.2.0-20231220.zip',
    'https://github.com/brechtsanders/winlibs_mingw/releases/download/2024-02-14/winlibs-x86_64-posix-seh-gcc-13.2.0-20240214.zip',
    'https://github.com/brechtsanders/winlibs_mingw/releases/download/2024-12-10/winlibs-x86_64-posix-seh-gcc-14.1.0-20241210.zip',
    'https://winlibs.com/downloads/winlibs-x86_64-posix-seh-gcc-13.2.0-20231220.zip'
  )
  $localZip = Join-Path (Get-Location) "tools\winlibs.zip"
  if (Test-Path $localZip) { $zipPath = $localZip } else {
    $zipPath = Join-Path $env:TEMP "winlibs_mingw.zip"
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    $downloaded = $false
    foreach ($u in $Urls) {
      try { Write-Host "Trying: $u"; Invoke-WebRequest -Uri $u -OutFile $zipPath -UseBasicParsing -ErrorAction Stop; $downloaded = $true; break } catch { Write-Host "Failed: $u" }
    }
    if (-not $downloaded) { Write-Error "Automatic downloads failed. Place tools\winlibs.zip manually."; return $false }
  }
  $extractDir = Join-Path $env:TEMP "winlibs_extracted"
  if (Test-Path $extractDir) { Remove-Item $extractDir -Recurse -Force }
  New-Item -ItemType Directory -Path $extractDir | Out-Null
  Write-Host "Extracting archive..."
  Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force
  $inner = Get-ChildItem -Path $extractDir | Where-Object { $_.PSIsContainer } | Select-Object -First 1
  if (-not $inner) { Write-Error "Could not find extracted root folder"; return $false }
  if (Test-Path $OutDir) { Remove-Item $OutDir -Recurse -Force }
  New-Item -ItemType Directory -Path $OutDir | Out-Null
  Get-ChildItem -Path $inner.FullName -Force | ForEach-Object { Move-Item -Path $_.FullName -Destination $OutDir -Force }
  if ($zipPath -notlike "*tools\winlibs.zip") { Remove-Item $zipPath -Force }
  Remove-Item $extractDir -Recurse -Force
  $gpp = Get-ChildItem -Path $OutDir -Recurse -Filter g++.exe -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($gpp) { Write-Host "Installed mingw-w64. Found g++ at: $($gpp.FullName)"; return $true } else { Write-Error "g++.exe not found under $OutDir"; return $false }
}

function Restore-MingwFromSystem {
  param([string]$Dest = "tools/mingw-w64")
  Write-Host "Locating g++.exe on the system..."
  $gppCmd = Get-Command g++ -ErrorAction SilentlyContinue
  $gppPath = $null
  if ($gppCmd) { $gppPath = $gppCmd.Source }
  if (-not $gppPath) {
    $searchRoots = @('C:\\msys64','C:\\MinGW','C:\\TDM-GCC','C:\\Program Files','C:\\Program Files (x86)')
    foreach ($r in $searchRoots) { if (Test-Path $r) { $found = Get-ChildItem -Path $r -Filter g++.exe -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1; if ($found) { $gppPath = $found.FullName; break } } }
  }
  if (-not $gppPath) { Write-Host "No g++.exe found on the system."; return $false }
  Write-Host "Found g++ at: $gppPath"
  $dest = Join-Path (Get-Location) $Dest
  if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
  New-Item -ItemType Directory -Path $dest | Out-Null
  $gppDir = Split-Path $gppPath -Parent
  if ($gppPath -match "\\usr\\bin\\g\+\+\.exe$") {
    $msysRoot = Split-Path $gppDir -Parent
    try { Copy-Item -Path (Join-Path $msysRoot 'usr') -Destination (Join-Path $dest 'usr') -Recurse -Force -ErrorAction Stop } catch { Write-Host "Warning: failed to copy usr: $_" }
    if (Test-Path (Join-Path $msysRoot 'mingw64')) { try { Copy-Item -Path (Join-Path $msysRoot 'mingw64') -Destination (Join-Path $dest 'mingw64') -Recurse -Force -ErrorAction Stop } catch { Write-Host "Warning: failed to copy mingw64: $_" } }
  } elseif ($gppPath -match "\\MinGW\\bin\\g\+\+\.exe$" -or $gppPath -match "\\TDM-GCC\\bin\\g\+\+\.exe$") {
    $root = Split-Path $gppDir -Parent
    try { Copy-Item -Path $root\* -Destination $dest -Recurse -Force -ErrorAction Stop } catch { Write-Host "Warning: failed to copy MinGW root: $_" }
  } else {
    try { Copy-Item -Path $gppDir -Destination (Join-Path $dest 'bin') -Recurse -Force -ErrorAction Stop } catch { Write-Host "Warning: failed to copy bin: $_" }
    $parent = Split-Path $gppDir -Parent
    if (Test-Path (Join-Path $parent 'lib')) { try { Copy-Item -Path (Join-Path $parent 'lib') -Destination (Join-Path $dest 'lib') -Recurse -Force -ErrorAction Stop } catch {} }
  }
  $gpp = Get-ChildItem -Path $dest -Recurse -Filter g++.exe -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($gpp) { Write-Host "Restored MinGW. Found g++ at: $($gpp.FullName)"; return $true } else { Write-Host "Copy completed but g++.exe not found under $dest"; return $false }
}

# --- end helpers ---

# Handle single-action switches early so user can invoke helpers directly
if ($FindCompiler) {
  $found = Find-Compiler
  if ($found -and $found.Count -gt 0) {
    Write-Host "Compilers found:"; $found | ForEach-Object { Write-Host " - $_" }
    exit 0
  }
  Write-Host "No compilers found."; exit 1
}

if ($InstallMingw) {
  $ok = Install-MingwFromWinlibs -OutDir "tools/mingw-w64"
  if ($ok) { Write-Host "MinGW installed."; exit 0 } else { Write-Error "MinGW install failed."; exit 1 }
}

if ($RestoreMingw) {
  $ok = Restore-MingwFromSystem -Dest "tools/mingw-w64"
  if ($ok) { Write-Host "MinGW restored."; exit 0 } else { Write-Error "MinGW restore failed."; exit 1 }
}

if ($SetupConan) {
  if (Get-Command conan -ErrorAction SilentlyContinue) {
    conan install . --output-folder $BuildDir --build missing; exit $LASTEXITCODE
  } else {
    python -m conan install . --output-folder $BuildDir --build missing; exit $LASTEXITCODE
  }
}

# If MinGW toolchain is vendored, add its bin to PATH so package builds find the compiler
$mingwDir = $null
if (Test-Path 'tools\mingw-w64') {
  $children = Get-ChildItem 'tools\mingw-w64' -Directory -ErrorAction SilentlyContinue
  if ($children -and $children.Count -eq 1) { $mingwDir = $children[0].FullName }
  else { $mingwDir = (Resolve-Path 'tools\mingw-w64').Path }
}
if ($mingwDir) {
  $mingwBin = Join-Path $mingwDir 'bin'
  if (Test-Path $mingwBin) { $env:PATH = "$mingwBin;$env:PATH"; Write-Host "Using mingw: $mingwDir" }
}

# Create build dir
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Path $BuildDir | Out-Null

# Install deps via Conan if requested
if ($UseConan -and -not $SkipDeps) {
  Write-Host "Running Conan install..."
  if (Get-Command conan -ErrorAction SilentlyContinue) {
    conan install . --output-folder $BuildDir --build missing
  }
  else {
    Write-Host "Conan not found in PATH; attempting python -m conan"
    python -m conan install . --output-folder $BuildDir --build missing
  }
}

# Choose toolchain file if generated by Conan
$toolchain = Join-Path $BuildDir 'conan_toolchain.cmake'
if (Test-Path $toolchain) { $tcArg = "-DCMAKE_TOOLCHAIN_FILE=$toolchain" } else { $tcArg = "" }

$cmakeArgs = @(
  '-S', '.',
  '-B', $BuildDir,
  '-G', 'Ninja',
  $tcArg,
  "-DCMAKE_BUILD_TYPE=$Config",
  '-DBUILD_BENCHMARKS=OFF',
  "-DBUILD_TESTING=ON"
) | Where-Object { $_ -ne '' }

Write-Host "Configuring CMake..."
cmake @cmakeArgs

Write-Host "Building..."
$jobs = [int][Math]::Max(1, [Math]::Round([Environment]::ProcessorCount * 1.5))
cmake --build $BuildDir --config $Config -- -j $jobs

Write-Host "To run tests: ctest --test-dir $BuildDir/tests --output-on-failure"

# Optionally run tests with mingw bin on PATH so DLLs are found
if ($RunTests) {
  Write-Host "Preparing to run tests..."
  if ($mingwBin -and (Test-Path $mingwBin)) { $env:PATH = "$mingwBin;$env:PATH"; Write-Host "Prepended mingw bin to PATH" }
  $ctestPath = Get-Command ctest -ErrorAction SilentlyContinue
  if ($ctestPath) {
    & $ctestPath.Source --test-dir "$BuildDir/tests" --output-on-failure
    exit $LASTEXITCODE
  }
  elseif (Test-Path "C:\Program Files\CMake\bin\ctest.exe") {
    & "C:\Program Files\CMake\bin\ctest.exe" --test-dir "$BuildDir/tests" --output-on-failure
    exit $LASTEXITCODE
  }
  else {
    Write-Error "ctest not found in PATH and not at 'C:\\Program Files\\CMake\\bin\\ctest.exe'"
    exit 2
  }
}
