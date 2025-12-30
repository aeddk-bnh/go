
cmake_minimum_required(VERSION 3.16)
project(GoGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options to make portable, fast builds
option(BUILD_TESTING "Build unit tests" OFF)
option(BUILD_BENCHMARKS "Build benchmark targets" OFF)
option(USE_CONAN "Prefer Conan-provided toolchain and deps when available" ON)

# Build the game executable by default
option(BUILD_GAME "Build the game executable (console)" ON)

# Default to Release when not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
