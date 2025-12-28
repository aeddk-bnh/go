add_library(gogame
  board.cpp
)

target_include_directories(gogame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(gogame PUBLIC cxx_std_17)
