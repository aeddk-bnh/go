#include "gtest/gtest.h"
#include "board.h"
#include "ai/mcts.h"

// Stress test: run parallel MCTS for a moderate workload to exercise threading and TT
TEST(MCTSTress, CompletesUnderLoad) {
  Board b(9);
  // create a slightly non-trivial position
  b.place(4,4,BLACK);
  b.place(3,4,WHITE);
  b.place(5,4,WHITE);

  MCTSConfig cfg;
  cfg.iterations = 2000;
  cfg.playout_depth = 100;
  MCTS m(cfg);

  // run with multiple threads
  auto mv = m.runParallel(b, BLACK, 2000, 4);
  // must return a legal move
  if(!mv.pass){
    EXPECT_GE(mv.x, 0);
    EXPECT_GE(mv.y, 0);
    EXPECT_LT(mv.x, b.size());
    EXPECT_LT(mv.y, b.size());
    EXPECT_EQ(b.get(mv.x,mv.y), EMPTY);
  }
}
