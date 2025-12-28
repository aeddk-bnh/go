#include "gtest/gtest.h"
#include "board.h"
#include "sgf.h"

TEST(SGFTest, ParseAndWriteRoundtrip){
  std::string s = "(;GM[1]FF[4]SZ[5]KM[6.5];B[aa];W[bb];B[];W[cc])";
  Board b(5);
  double komi=0;
  ASSERT_TRUE(SGF::parse(s, b, komi));
  EXPECT_DOUBLE_EQ(komi, 6.5);
  EXPECT_EQ(b.get(0,0), BLACK);
  EXPECT_EQ(b.get(1,1), WHITE);
  // write back
  std::string out = SGF::write(b, komi);
  Board b2(5);
  double komi2 = 0;
  ASSERT_TRUE(SGF::parse(out, b2, komi2));
  EXPECT_DOUBLE_EQ(komi2, komi);
  // compare grids
  for(int y=0;y<5;y++) for(int x=0;x<5;x++) EXPECT_EQ(b.get(x,y), b2.get(x,y));
  EXPECT_EQ(b.moves().size(), b2.moves().size());
}
