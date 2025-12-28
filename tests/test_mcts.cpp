#include "gtest/gtest.h"
#include "board.h"
#include "ai/mcts.h"

TEST(MCTSTest, RunsAndReturnsLegal) {
  Board b(5);
  MCTSConfig cfg; cfg.iterations = 100; cfg.playout_depth = 50;
  MCTS m(cfg);
  auto mv = m.run(b, BLACK);
  // move should be either pass or on-board
  if(!mv.pass){
    EXPECT_GE(mv.x, 0);
    EXPECT_GE(mv.y, 0);
    EXPECT_LT(mv.x, b.size());
    EXPECT_LT(mv.y, b.size());
  }
}

TEST(MCTSTest, SelfPlayFewMoves) {
  Board b(5);
  MCTSConfig cfg; cfg.iterations = 80;
  MCTS m(cfg);
  for(int i=0;i<6;i++){
    Stone p = (i%2==0?BLACK:WHITE);
    auto mv = m.run(b,p);
    if(mv.pass) { b.pass(p); }
    else { EXPECT_TRUE(b.place(mv.x,mv.y,p)); }
  }
}
