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
    // now move root to child (persistent tree)
    EXPECT_TRUE(m.moveToChild(mv));
    // root hash should equal board zobrist
    EXPECT_EQ(m.rootHash(), b.zobrist());
  }
}

TEST(MCTSTest, VirtualLossBiasesSelection){
  Board b(5);
  MCTSConfig cfg; cfg.iterations = 200;
  MCTS m(cfg);
  // run to build some children
  auto mv = m.run(b, BLACK);
  EXPECT_GT(m.rootChildrenCount(), 0);
  int idx0 = 0;
  // apply virtual loss to child 0
  EXPECT_TRUE(m.applyVirtualLossToChildIndex(idx0, 5));
  int chosen = m.chooseChildIndexAtRoot();
  // chosen should not be 0 (since virtual loss penalizes it)
  EXPECT_NE(chosen, idx0);
  // revert
  EXPECT_TRUE(m.revertVirtualLossFromChildIndex(idx0, 5));
}

