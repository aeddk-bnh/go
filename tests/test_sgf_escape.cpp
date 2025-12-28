#include "gtest/gtest.h"
#include "sgf.h"
#include "board.h"

TEST(SGFEscapeTest, RoundtripEscapeSequences) {
  SGF::Game g;
  g.PB = "Black;Name";
  g.PW = "White\\Name";
  g.RE = "B+R";
  g.KM = 6.5;
  // add a move with comment containing newline, semicolon and backslash
  Board::Move m{3,3,BLACK,false, std::string("Line1\nLine2; semicolon and back\\slash") };
  g.moves.push_back(m);

  std::string out = SGF::write(g);
  Board b(19);
  SGF::Game g2;
  double komi=0;
  ASSERT_TRUE(SGF::parse(out, b, komi, &g2));
  ASSERT_EQ(g2.moves.size(), 1);
  EXPECT_EQ(g2.moves[0].comment, m.comment);
  EXPECT_EQ(g2.PB, g.PB);
  EXPECT_EQ(g2.PW, g.PW);
}

TEST(SGFEscapeTest, ParseEscapedSequencesInInput){
  std::string s = "(;GM[1]FF[4]SZ[5]KM[0];B[aa]C[Line\\nNext\\;Part\\\\End])"; // C with escaped newline, semicolon and backslash
  Board b(5);
  SGF::Game g;
  double komi=0;
  ASSERT_TRUE(SGF::parse(s,b,komi,&g));
  ASSERT_EQ(g.moves.size(),1);
  EXPECT_EQ(g.moves[0].comment, std::string("Line\nNext;Part\\End"));
}
