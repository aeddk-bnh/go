#include "board.h"
#include <stack>

Board::Board(int n): N(n), grid(n*n, EMPTY) {}

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
  grid[id] = s;
  // Check adjacent enemy groups for capture
  const int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
  for(int i=0;i<4;i++){
    int nx=x+dx[i], ny=y+dy[i];
    if(!inside(nx,ny)) continue;
    int nid = idx(nx,ny);
    Stone occ = grid[nid];
    if(occ!=EMPTY && occ!=s){
      std::vector<char> visited(N*N,0);
      if(!hasLibertyDFS(nx,ny,visited)){
        removeGroup(nx,ny,occ);
      }
    }
  }
  // Check self-capture
  std::vector<char> visited(N*N,0);
  if(!hasLibertyDFS(x,y,visited)){
    // undo
    grid[id] = EMPTY;
    return false;
  }
  return true;
}