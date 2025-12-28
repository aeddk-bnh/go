#include "gtest/gtest.h"
#include "board.h"
#include "sgf.h"

TEST(SGFMetaTest, MetadataAndComments){
  std::string s = "(;GM[1]FF[4]SZ[5]KM[7.5]PB[BlackPlayer]PW[WhitePlayer]RE[B+R];B[aa]C[Opening];W[bb];B[cc]C[Good move])";
  Board b(5);
  SGF::Game g;
  double komi=0;
  ASSERT_TRUE(SGF::parse(s,b,komi,&g));
  EXPECT_DOUBLE_EQ(komi, 7.5);
  EXPECT_EQ(g.PB, "BlackPlayer");
  EXPECT_EQ(g.PW, "WhitePlayer");
  EXPECT_EQ(g.RE, "B+R");
  ASSERT_EQ(g.moves.size(), 3);
  EXPECT_EQ(g.moves[0].comment, "Opening");
  EXPECT_EQ(g.moves[2].comment, "Good move");

  // Write file and parse again
  std::string out = SGF::write(g);
  Board b2(5);
  SGF::Game g2;
  double komi2=0;
  ASSERT_TRUE(SGF::parse(out, b2, komi2, &g2));
  EXPECT_EQ(g2.PB, g.PB);
  EXPECT_EQ(g2.moves[0].comment, g.moves[0].comment);
}
