#include "board.h"
#include <stack>
#include <cstdint>

Board::Board(int n): N(n), grid(n*n, EMPTY), zobristTable(n) {
  currentHash = zobristTable.hash(grid);
  hashHistory.push_back(currentHash);
}

bool Board::inside(int x,int y) const { return x>=0 && y>=0 && x<N && y<N; }
int Board::idx(int x,int y) const { return y*N + x; }

bool Board::hasLibertyDFS(int x,int y, std::vector<char>& visited) const {
  Stone c = grid[idx(x,y)];
  if (c==EMPTY) return true;
  std::stack<std::pair<int,int>> st;
  st.push({x,y}); visited[idx(x,y)] = 1;
  while(!st.empty()){
    auto [cx,cy] = st.top(); st.pop();
    const int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
    for(int i=0;i<4;i++){
      int nx = cx+dx[i], ny = cy+dy[i];
      if(!inside(nx,ny)) continue;
      int id = idx(nx,ny);
      if(grid[id]==EMPTY) return true;
      if(grid[id]==c && !visited[id]){ visited[id]=1; st.push({nx,ny}); }
    }
  }
  return false;
}

void Board::removeGroup(int x,int y, Stone color){
  std::stack<std::pair<int,int>> st;
  st.push({x,y});
  std::vector<char> seen(N*N,0);
  seen[idx(x,y)]=1;
  while(!st.empty()){
    auto [cx,cy] = st.top(); st.pop();
    grid[idx(cx,cy)] = EMPTY;
    const int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
    for(int i=0;i<4;i++){
      int nx=cx+dx[i], ny=cy+dy[i];
      if(!inside(nx,ny)) continue;
      int id=idx(nx,ny);
      if(!seen[id] && grid[id]==color){ seen[id]=1; st.push({nx,ny}); }
    }
  }
}

bool Board::place(int x,int y, Stone s){
  if(!inside(x,y)) return false;
  int id = idx(x,y);
  if(grid[id] != EMPTY) return false;

  // Work on a simulated grid to check legality (suicide & superko) before modifying real board
  std::vector<Stone> tmp = grid;
  tmp[id] = s;

  auto hasLibertyInGrid = [&](int sx,int sy, const std::vector<Stone>& g)->bool{
    if(g[idx(sx,sy)]==EMPTY) return true;
    std::vector<char> visited(N*N,0);
    std::stack<std::pair<int,int>> st;
    st.push({sx,sy}); visited[idx(sx,sy)]=1;
    while(!st.empty()){
      auto [cx,cy]=st.top(); st.pop();
      const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
      for(int i=0;i<4;i++){
        int nx=cx+dx[i], ny=cy+dy[i];
        if(!inside(nx,ny)) continue;
        int nid = idx(nx,ny);
        if(g[nid]==EMPTY) return true;
        if(g[nid]==g[idx(sx,sy)] && !visited[nid]){ visited[nid]=1; st.push({nx,ny}); }
      }
    }
    return false;
  };

  auto collectGroupInGrid = [&](int sx,int sy, const std::vector<Stone>& g){
    std::vector<int> out;
    Stone c = g[idx(sx,sy)];
    std::stack<std::pair<int,int>> st;
    st.push({sx,sy});
    std::vector<char> seen(N*N,0);
    seen[idx(sx,sy)]=1;
    while(!st.empty()){
      auto [cx,cy] = st.top(); st.pop();
      out.push_back(idx(cx,cy));
      const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
      for(int i=0;i<4;i++){
        int nx=cx+dx[i], ny=cy+dy[i];
        if(!inside(nx,ny)) continue;
        int nid = idx(nx,ny);
        if(!seen[nid] && g[nid]==c){ seen[nid]=1; st.push({nx,ny}); }
      }
    }
    return out;
  };

  // Check and remove enemy groups with no liberties on tmp
  const int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
  for(int i=0;i<4;i++){
    int nx=x+dx[i], ny=y+dy[i];
    if(!inside(nx,ny)) continue;
    int nid = idx(nx,ny);
    if(tmp[nid]!=EMPTY && tmp[nid]!=s){
      if(!hasLibertyInGrid(nx,ny,tmp)){
        auto gang = collectGroupInGrid(nx,ny,tmp);
        for(int p : gang) tmp[p] = EMPTY;
      }
    }
  }

  // Check suicide
  if(!hasLibertyInGrid(x,y,tmp)) return false;

  // Check superko (hash exists previously)
  uint64_t newHash = zobristTable.hash(tmp);
  for(uint64_t h : hashHistory) if(h==newHash) return false;

  // Accept move: apply tmp to real grid
  grid.swap(tmp);
  currentHash = newHash;
  hashHistory.push_back(currentHash);
  recordMove(x,y,s,false);
  return true;
}

bool Board::pass(Stone s){
  // pass does not change grid but counts as a move
  hashHistory.push_back(currentHash);
  recordPass(s);
  return true;
}

// public helper implementations
void Board::appendMove(int x,int y, Stone s, bool pass, const std::string &comment){
  moveHistory.push_back({x,y,s,pass,comment});
}

void Board::setLastMoveComment(const std::string &c){
  if(!moveHistory.empty()) moveHistory.back().comment = c;
}