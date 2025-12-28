# Benchmark CMake


# Prefer a system-installed Google Benchmark. If not available, skip the
# richer benchmark target to avoid fetching/building third-party sources
# in CI-less environments. We still provide `bench_mcts_simple` always.
find_package(benchmark QUIET)
if(benchmark_FOUND)
  add_executable(bench_mcts bench_mcts.cpp)
  target_link_libraries(bench_mcts PRIVATE gogame benchmark::benchmark)
  target_include_directories(bench_mcts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
else()
  message(STATUS "Google Benchmark not found; skipping bench_mcts target.")
endif()

add_executable(bench_mcts_simple bench_mcts_simple.cpp)
target_link_libraries(bench_mcts_simple PRIVATE gogame)
target_include_directories(bench_mcts_simple PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
