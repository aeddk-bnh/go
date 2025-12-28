#include "gtest/gtest.h"
#include "board.h"

TEST(BoardTest, SimpleCapture) {
  Board b(5);
  // Black surrounds white at (1,1)
  EXPECT_TRUE(b.place(1,0,BLACK));
  EXPECT_TRUE(b.place(0,1,BLACK));
  EXPECT_TRUE(b.place(2,1,BLACK));
  EXPECT_TRUE(b.place(1,2,BLACK));

  // Placing white in the surrounded point is a suicide and should be rejected
  EXPECT_FALSE(b.place(1,1,WHITE));
  EXPECT_EQ(b.get(1,1), EMPTY);
}
