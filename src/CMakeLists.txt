add_library(gogame
  board.cpp
  zobrist.cpp
  rules.cpp
  sgf.cpp
  ai/mcts.cpp
  ai/mcts_extra.cpp
)
add_subdirectory(ai)
add_subdirectory(bench)

target_include_directories(gogame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(gogame PUBLIC cxx_std_17)
