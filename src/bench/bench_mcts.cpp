#include <benchmark/benchmark.h>
#include "board.h"
#include "ai/mcts.h"

static void BM_MCTS_Parallel(benchmark::State& state) {
  int threads = state.range(0);
  int iterations = state.range(1);
  Board b(9);
  b.place(4,4,BLACK);
  b.place(3,4,WHITE);
  b.place(5,4,WHITE);
  MCTSConfig cfg; cfg.iterations = iterations; cfg.playout_depth = 50;
  MCTS m(cfg);
  for (auto _ : state) {
    auto mv = m.runParallel(b, BLACK, iterations, threads);
    benchmark::DoNotOptimize(mv);
  }
}

BENCHMARK(BM_MCTS_Parallel)->Args({1,200})->Args({2,400})->Args({4,800})->Args({8,1600});
BENCHMARK_MAIN();
