# add AI sources here
add_library(ai STATIC mcts.cpp mcts_extra.cpp tt_sharded.cpp pvn.cpp)
target_include_directories(ai PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ai PUBLIC gogame)

# Link ai into gogame static lib by adding source in parent CMakeLists (if desired)
