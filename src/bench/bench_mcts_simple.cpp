#include <chrono>
#include <iostream>
#include "board.h"
#include "ai/mcts.h"

using namespace std::chrono;

void run_case(int threads, int iterations) {
  Board b(9);
  b.place(4,4,BLACK);
  b.place(3,4,WHITE);
  b.place(5,4,WHITE);
  MCTSConfig cfg; cfg.iterations = iterations; cfg.playout_depth = 50;
  MCTS m(cfg);
  auto start = high_resolution_clock::now();
  auto mv = m.runParallel(b, BLACK, iterations, threads);
  auto end = high_resolution_clock::now();
  auto ms = duration_cast<milliseconds>(end - start).count();
  std::cout << "threads=" << threads << " iterations=" << iterations << " time_ms=" << ms << "\n";
}

int main(int argc, char** argv) {
  if (argc == 3) {
    int threads = std::stoi(argv[1]);
    int iterations = std::stoi(argv[2]);
    run_case(threads, iterations);
    return 0;
  }
  // Default cases
  run_case(1, 200);
  run_case(2, 400);
  run_case(4, 800);
  run_case(8, 1600);
  return 0;
}
