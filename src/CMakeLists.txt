add_library(gogame
  board.cpp
  zobrist.cpp
  rules.cpp
  sgf.cpp
)
add_subdirectory(ai)
add_subdirectory(bench)

target_include_directories(gogame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Link the ai static library into the main gogame library so all AI symbols are available
target_link_libraries(gogame PUBLIC ai)

target_compile_features(gogame PUBLIC cxx_std_17)
