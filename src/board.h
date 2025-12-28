#pragma once

#include <vector>
#include <cstdint>

#include "types.h"
#include "zobrist.h"

class Board {
public:
  Board(int n = 9);
  bool inside(int x,int y) const;
  int idx(int x,int y) const;
  bool place(int x,int y, Stone s); // returns true if move placed
  void set(int x,int y, Stone s){ grid[idx(x,y)] = s; }
  Stone get(int x,int y) const { return grid[idx(x,y)]; }
  int size() const { return N; }
  uint64_t zobrist() const { return currentHash; }
  const std::vector<uint64_t>& history() const { return hashHistory; }
private:
  int N;
  std::vector<Stone> grid;
  bool hasLibertyDFS(int x,int y, std::vector<char>& visited) const;
  void removeGroup(int x,int y, Stone color);
  // Zobrist hashing & history for superko
  uint64_t currentHash{0};
  std::vector<uint64_t> hashHistory;
  Zobrist zobristTable{9};
};