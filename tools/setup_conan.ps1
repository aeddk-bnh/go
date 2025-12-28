param(
  [string]$BuildDir = "build",
  [string]$Generator = "Ninja"
)

Write-Host "Running Conan install..."
conan install . --output-folder $BuildDir --build missing

Write-Host "Configuring CMake with Conan toolchain..."
cmake -S . -B $BuildDir -G $Generator -DCMAKE_TOOLCHAIN_FILE=$BuildDir\conan_toolchain.cmake

Write-Host "Building..."
cmake --build $BuildDir -- -j 6

Write-Host "Done. To run tests: ctest --test-dir $BuildDir --output-on-failure"
