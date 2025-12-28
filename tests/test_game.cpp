#include <gtest/gtest.h>
#include "board.h"
#include "game.h"

// Helper to set up a small board state
TEST(GameRules, SimpleCapture){
  Board b(5);
  // black at (1,1), white surrounding and then black gets captured
  EXPECT_TRUE(b.place(1,1,BLACK));
  EXPECT_TRUE(b.place(0,1,WHITE));
  EXPECT_TRUE(b.place(1,0,WHITE));
  EXPECT_TRUE(b.place(2,1,WHITE));
  EXPECT_TRUE(b.place(1,2,WHITE));
  // black at (1,1) should have been captured by last white move
  EXPECT_EQ(b.get(1,1), EMPTY);
}

TEST(GameRules, MultiStoneCapture){
  Board b(5);
  // create two connected black stones surrounded
  EXPECT_TRUE(b.place(1,1,BLACK));
  EXPECT_TRUE(b.place(2,1,BLACK));
  EXPECT_TRUE(b.place(0,1,WHITE));
  EXPECT_TRUE(b.place(1,0,WHITE));
  EXPECT_TRUE(b.place(2,0,WHITE));
  EXPECT_TRUE(b.place(3,1,WHITE));
  EXPECT_TRUE(b.place(2,2,WHITE));
  EXPECT_TRUE(b.place(1,2,WHITE));
  // both black stones captured
  EXPECT_EQ(b.get(1,1), EMPTY);
  EXPECT_EQ(b.get(2,1), EMPTY);
}

TEST(GameRules, SuicideIllegal){
  Board b(5);
  // surround a point, attempt suicide
  EXPECT_TRUE(b.place(0,1,WHITE));
  EXPECT_TRUE(b.place(1,0,WHITE));
  EXPECT_TRUE(b.place(2,1,WHITE));
  EXPECT_TRUE(b.place(1,2,WHITE));
  // black tries to play at 1,1 which would be suicide
  EXPECT_FALSE(b.place(1,1,BLACK));
  // but isLegal should report false
  EXPECT_FALSE(b.isLegal(1,1,BLACK));
}

TEST(GameRules, KoSimple){
  Board b(5);
  // set up a clear single-stone capture and check immediate recapture is illegal
  EXPECT_TRUE(b.place(1,1,BLACK));
  EXPECT_TRUE(b.place(0,1,WHITE));
  EXPECT_TRUE(b.place(1,0,WHITE));
  EXPECT_TRUE(b.place(2,1,WHITE));
  EXPECT_TRUE(b.place(1,2,WHITE));
  // black at (1,1) should have been captured by last white move
  EXPECT_EQ(b.get(1,1), EMPTY);
  // immediate recapture should be illegal (superko)
  EXPECT_FALSE(b.isLegal(1,1,BLACK));
}

TEST(GameRules, PassAndResignAndGame){
  Game g(5);
  EXPECT_EQ(g.currentPlayer(), BLACK);
  EXPECT_TRUE(g.pass());
  EXPECT_EQ(g.currentPlayer(), WHITE);
  EXPECT_TRUE(g.pass());
  EXPECT_TRUE(g.isOver());
  // new game, resign
  Game g2(5);
  EXPECT_TRUE(g2.play(0,0));
  g2.resign();
  EXPECT_TRUE(g2.isOver());
  EXPECT_EQ(g2.winner(), WHITE);
}

