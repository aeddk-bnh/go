add_library(gogame
  board.cpp
  zobrist.cpp
  rules.cpp
  sgf.cpp
  game.cpp
)
add_subdirectory(ai)
add_subdirectory(bench)

target_include_directories(gogame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Do not create a circular link between `gogame` and `ai`.
# The `ai` library depends on `gogame` (it uses board/rules), so we keep
# `gogame` independent and let executables (or tests/benches) link both
# libraries as needed.

target_compile_features(gogame PUBLIC cxx_std_17)
