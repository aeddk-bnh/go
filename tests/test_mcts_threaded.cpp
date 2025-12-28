#include "gtest/gtest.h"
#include "ai/mcts.h"

TEST(MCTSThreaded, CompletesAndReturnsLegalMove) {
  Board b(5);
  // some stones to make it non-trivial
  b.place(2,2,BLACK);
  b.place(1,2,WHITE);

  MCTSConfig cfg;
  cfg.iterations = 500;
  MCTS m(cfg);

  auto mv = m.runParallel(b, BLACK, 500, 4);
  // move must be either pass or empty intersection
  if (!mv.pass) {
    EXPECT_GE(mv.x, 0);
    EXPECT_GE(mv.y, 0);
    EXPECT_LT(mv.x, 5);
    EXPECT_LT(mv.y, 5);
    EXPECT_EQ(b.get(mv.x,mv.y), EMPTY);
  }
}
