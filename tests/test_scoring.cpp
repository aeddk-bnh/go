#include "gtest/gtest.h"
#include "board.h"
#include "rules.h"

TEST(ScoringTest, JapaneseSimpleTerritory) {
  Board b(5);
  // Make a 3x3 ring around (2,2)
  std::vector<std::pair<int,int>> blacks = {{1,1},{1,2},{1,3},{2,1},{2,3},{3,1},{3,2},{3,3}};
  for(auto [x,y] : blacks) EXPECT_TRUE(b.place(x,y,BLACK));
  // verify stones count
  int stones=0; for(int yy=0;yy<5;yy++) for(int xx=0;xx<5;xx++) if(b.get(xx,yy)==BLACK) stones++;
  EXPECT_EQ(stones, 8);
  // Center (2,2) is territory for black
  auto sc = Scorer::score(b, Ruleset::Japanese, 0.0);
  if (sc.first != 1.0) {
    // debug print
    for(int yy=0;yy<5;yy++){
      for(int xx=0;xx<5;xx++){
        char c = (b.get(xx,yy)==BLACK?'B':(b.get(xx,yy)==WHITE?'W':'.'));
        std::cout<<c;
      }
      std::cout<<"\n";
    }
  }
  EXPECT_DOUBLE_EQ(sc.first, 1.0); // black territory
  EXPECT_DOUBLE_EQ(sc.second, 0.0); // white territory + komi
}

TEST(ScoringTest, ChineseAreaCountsStonesAndTerritory){
  Board b(5);
  // Same ring of black
  std::vector<std::pair<int,int>> blacks = {{1,1},{1,2},{1,3},{2,1},{2,3},{3,1},{3,2},{3,3}};
  for(auto [x,y] : blacks) EXPECT_TRUE(b.place(x,y,BLACK));
  auto sc = Scorer::score(b, Ruleset::Chinese, 0.0);
  // stones = 8, territory center =1 -> area = 9
  EXPECT_DOUBLE_EQ(sc.first, 9.0);
  EXPECT_DOUBLE_EQ(sc.second, 0.0);
}
