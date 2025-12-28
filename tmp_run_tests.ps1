$mingw='C:\Users\ASUS\Documents\go\tools\mingw-w64\bin'
$env:PATH = "$mingw;$env:PATH"
Write-Host "PATH prepped: $env:PATH"
& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build/tests --output-on-failure
