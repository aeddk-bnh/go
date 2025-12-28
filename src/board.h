#pragma once

#include <vector>

enum Stone { EMPTY = 0, BLACK = 1, WHITE = 2 };

class Board {
public:
  Board(int n = 9);
  bool inside(int x,int y) const;
  int idx(int x,int y) const;
  bool place(int x,int y, Stone s); // returns true if move placed
  void set(int x,int y, Stone s){ grid[idx(x,y)] = s; }
  Stone get(int x,int y) const { return grid[idx(x,y)]; }
  int size() const { return N; }
private:
  int N;
  std::vector<Stone> grid;
  bool hasLibertyDFS(int x,int y, std::vector<char>& visited) const;
  void removeGroup(int x,int y, Stone color);
};