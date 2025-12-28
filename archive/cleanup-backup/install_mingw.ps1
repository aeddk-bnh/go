Param(
    [string[]]$Urls = @(
        'https://github.com/brechtsanders/winlibs_mingw/releases/download/2023-12-20/winlibs-x86_64-posix-seh-gcc-13.2.0-20231220.zip',
        'https://github.com/brechtsanders/winlibs_mingw/releases/download/2024-02-14/winlibs-x86_64-posix-seh-gcc-13.2.0-20240214.zip',
        'https://github.com/brechtsanders/winlibs_mingw/releases/download/2024-12-10/winlibs-x86_64-posix-seh-gcc-14.1.0-20241210.zip',
        'https://winlibs.com/downloads/winlibs-x86_64-posix-seh-gcc-13.2.0-20231220.zip'
    ),
    [string]$OutDir = "tools/mingw-w64"
)

$ErrorActionPreference = 'Stop'

Write-Host "If you have a winlibs zip already, place it at tools\\winlibs.zip and rerun this script."
$localZip = Join-Path (Get-Location) "tools\winlibs.zip"
if (Test-Path $localZip) {
    Write-Host "Found local archive at $localZip; using that."
    Copy-Item -Path $localZip -Destination $zipPath -Force
} else {
    Write-Host "No local archive found. Attempting to download from a list of known URLs..."
    $zipPath = Join-Path $env:TEMP "winlibs_mingw.zip"
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    $downloaded = $false
    foreach ($u in $Urls) {
        try {
            Write-Host "Trying: $u"
            Invoke-WebRequest -Uri $u -OutFile $zipPath -UseBasicParsing -ErrorAction Stop
            $downloaded = $true
            break
        } catch {
            Write-Host "Failed: $u"
        }
    }
    if (-not $downloaded) {
        Write-Host "Automatic downloads failed. Please download a Winlibs archive manually and save it as tools\\winlibs.zip"
        Write-Host "Example sources:"
        Write-Host "  - https://github.com/brechtsanders/winlibs_mingw/releases"
        Write-Host "  - https://winlibs.com/"
        exit 4
    }
}

$extractDir = Join-Path $env:TEMP "winlibs_extracted"
if (Test-Path $extractDir) { Remove-Item $extractDir -Recurse -Force }
New-Item -ItemType Directory -Path $extractDir | Out-Null

Write-Host "Extracting archive..."
Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force

Write-Host "Locating top-level folder inside the archive..."
$inner = Get-ChildItem -Path $extractDir | Where-Object { $_.PSIsContainer } | Select-Object -First 1
if (-not $inner) { Write-Error "Could not find extracted root folder"; exit 2 }

if (Test-Path $OutDir) {
    Write-Host "Removing existing $OutDir"
    Remove-Item $OutDir -Recurse -Force
}

Write-Host "Creating $OutDir and moving files..."
New-Item -ItemType Directory -Path $OutDir | Out-Null
Get-ChildItem -Path $inner.FullName -Force | ForEach-Object {
    Move-Item -Path $_.FullName -Destination $OutDir -Force
}

Write-Host "Cleaning temporary files..."
Remove-Item $zipPath -Force
Remove-Item $extractDir -Recurse -Force

$gpp = Get-ChildItem -Path $OutDir -Recurse -Filter g++.exe -ErrorAction SilentlyContinue | Select-Object -First 1
if ($gpp) {
    Write-Host "Installed mingw-w64. Found g++ at: $($gpp.FullName)"
    Write-Host "Next: run CMake pointing to this compiler, e.g."
    Write-Host "cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake -DCMAKE_MAKE_PROGRAM=\"$([Environment]::GetFolderPath('UserProfile'))\\.conan2\\p\\ninja*/p/bin/ninja.exe\" -DCMAKE_CXX_COMPILER=\"$($gpp.FullName)\" -DCMAKE_BUILD_TYPE=Release"
} else {
    Write-Error "Download succeeded but g++.exe was not found under $OutDir"
    exit 3
}

Write-Host "Done."
