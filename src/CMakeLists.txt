add_library(gogame
  board.cpp
  zobrist.cpp
  rules.cpp
  sgf.cpp
  game.cpp
)
add_subdirectory(ai)
add_subdirectory(bench)

# Optionally build the console game executable
option(BUILD_GAME "Build console game executable" ON)
if(BUILD_GAME)
  add_executable(go_console main.cpp)
  target_link_libraries(go_console PRIVATE gogame)
  target_compile_features(go_console PRIVATE cxx_std_17)
endif()

target_include_directories(gogame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Do not create a circular link between `gogame` and `ai`.
# The `ai` library depends on `gogame` (it uses board/rules), so we keep
# `gogame` independent and let executables (or tests/benches) link both
# libraries as needed.

target_compile_features(gogame PUBLIC cxx_std_17)
