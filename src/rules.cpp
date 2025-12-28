#include "rules.h"
#include <vector>
#include <queue>
#include <set>
#include <iostream>

std::pair<double,double> Scorer::score(const Board& b, Ruleset r, double komi){
  int N = b.size();
  int stones_black = 0, stones_white = 0;
  for(int y=0;y<N;y++) for(int x=0;x<N;x++){
    auto s = b.get(x,y);
    if(s==BLACK) stones_black++;
    else if(s==WHITE) stones_white++;
  }

  std::vector<char> seen(N*N,0);
  int territory_black = 0, territory_white = 0;

  auto idx = [&](int x,int y){ return y*N + x; };
  const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};

  for(int y=0;y<N;y++){
    for(int x=0;x<N;x++){
      if(b.get(x,y)!=EMPTY) continue;
      int id = idx(x,y);
      if(seen[id]) continue;
      // BFS this empty region
      std::queue<std::pair<int,int>> q;
      std::vector<int> region;
      std::set<Stone> borders;
      bool touches_edge = false;
      q.push({x,y}); seen[id]=1;
      while(!q.empty()){
        auto [cx,cy] = q.front(); q.pop();
        region.push_back(idx(cx,cy));
        for(int k=0;k<4;k++){
          int nx=cx+dx[k], ny=cy+dy[k];
          if(nx<0||ny<0||nx>=N||ny>=N){ touches_edge = true; continue; }
          auto g = b.get(nx,ny);
          if(g==EMPTY){
            int nid = idx(nx,ny);
            if(!seen[nid]){ seen[nid]=1; q.push({nx,ny}); }
          } else {
            borders.insert(g);
          }
        }
      }
      // Determine owner (region must not touch board edge)
      if(!touches_edge && borders.size()==1){
        if(*borders.begin()==BLACK) {
          territory_black += (int)region.size();
        }
        else if(*borders.begin()==WHITE) {
          territory_white += (int)region.size();
        }
      }
    }
  }

  double black_score=0, white_score=0;
  // (debug output removed for cleaner benchmark/test runs)
  if(r==Ruleset::Chinese){
    black_score = stones_black + territory_black;
    white_score = stones_white + territory_white + komi;
  } else { // Japanese: territory only (simplified, not counting captured stones)
    black_score = territory_black;
    white_score = territory_white + komi;
  }
  return {black_score, white_score};
}