# add AI sources here
add_library(ai STATIC mcts.cpp mcts_extra.cpp tt_sharded.cpp pvn.cpp)
target_include_directories(ai PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
## `ai` depends on the core game library for types and rules; link against it.
target_link_libraries(ai PRIVATE gogame)

# Note: do NOT link `gogame` -> `ai` (would create a circular dependency).
