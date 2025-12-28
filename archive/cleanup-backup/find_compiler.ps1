# Search for common C++ compilers on Windows
$candidates = @()

# Check PATH for commands
try {
    $g = Get-Command g++ -ErrorAction SilentlyContinue
    if ($g) { $candidates += $g.Source }
} catch {}
try {
    $c = Get-Command clang++ -ErrorAction SilentlyContinue
    if ($c) { $candidates += $c.Source }
} catch {}
try {
    $cl = Get-Command cl -ErrorAction SilentlyContinue
    if ($cl) { $candidates += $cl.Source }
} catch {}

# Known install locations
$known = @(
    'C:\msys64\mingw64\bin\g++.exe',
    'C:\msys64\usr\bin\g++.exe',
    'C:\MinGW\bin\g++.exe',
    'C:\TDM-GCC\bin\g++.exe',
    'C:\Program Files\LLVM\bin\clang++.exe',
    'C:\Program Files\LLVM\bin\clang.exe',
    'C:\Program Files (x86)\LLVM\bin\clang++.exe',
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC',
    'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC'
)

foreach ($k in $known) {
    if (Test-Path $k) { $candidates += (Get-Item $k).FullName }
    else {
        # If path is a directory, try to find cl.exe inside
        if ((Test-Path $k) -and (Get-Item $k).PSIsContainer) {
            $found = Get-ChildItem -Path $k -Recurse -Filter cl.exe -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($found) { $candidates += $found.FullName }
        }
    }
}

$candidates = $candidates | Select-Object -Unique
if ($candidates.Count -eq 0) {
    Write-Host "No compilers found in PATH or common install locations."
    Write-Host "You can place a portable winlibs zip at tools/winlibs.zip and run install_mingw.ps1, or install Visual Studio / MSVC or MinGW."
    exit 1
}

Write-Host "Found the following compiler executables:"
foreach ($p in $candidates) { Write-Host " - $p" }

Write-Host 'To use one with CMake, set -DCMAKE_CXX_COMPILER="<path>"'
