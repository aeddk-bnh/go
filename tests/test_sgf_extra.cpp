#include "gtest/gtest.h"
#include "board.h"
#include "sgf.h"

TEST(SGFExtraTest, PassOnlySequence) {
  std::string s = "(;GM[1]FF[4]SZ[5]KM[0];B[];W[];B[];W[];B[] )";
  Board b(5);
  double komi=0;
  ASSERT_TRUE(SGF::parse(s, b, komi));
  EXPECT_DOUBLE_EQ(komi, 0.0);
  // all moves should be recorded as passes
  auto mv = b.moves();
  EXPECT_EQ(mv.size(), 5);
  for(auto &m : mv) EXPECT_TRUE(m.pass);

  std::string out = SGF::write(b, komi);
  Board b2(5);
  double komi2=0;
  ASSERT_TRUE(SGF::parse(out, b2, komi2));
  EXPECT_EQ(b.moves().size(), b2.moves().size());
  for(size_t i=0;i<b.moves().size();++i){
    EXPECT_EQ(b.moves()[i].pass, b2.moves()[i].pass);
    EXPECT_EQ(b.moves()[i].s, b2.moves()[i].s);
  }
}

TEST(SGFExtraTest, LargeBoardRoundtrip) {
  // Create a short game on 19x19
  std::string s = "(;GM[1]FF[4]SZ[19]KM[6.5]";
  std::vector<std::pair<int,int>> moves = {{3,3},{15,15},{16,3},{3,16},{10,10}};
  std::vector<char> colors = {'B','W','B','W','B'};
  for(size_t i=0;i<moves.size();++i){
    s += ";";
    s += colors[i];
    s += "[";
    s += char('a'+moves[i].first);
    s += char('a'+moves[i].second);
    s += "]";
  }
  s += ")";

  Board b(19);
  double komi=0;
  ASSERT_TRUE(SGF::parse(s, b, komi));
  EXPECT_DOUBLE_EQ(komi, 6.5);
  EXPECT_EQ(b.moves().size(), moves.size());

  std::string out = SGF::write(b, komi);
  Board b2(19);
  double komi2=0;
  ASSERT_TRUE(SGF::parse(out, b2, komi2));
  EXPECT_DOUBLE_EQ(komi2, komi);
  EXPECT_EQ(b.moves().size(), b2.moves().size());

  // Compare grid positions for moves
  for(size_t i=0;i<moves.size();++i){
    auto m = b.moves()[i];
    if(!m.pass) EXPECT_EQ(b2.get(m.x,m.y), b.get(m.x,m.y));
  }
}
