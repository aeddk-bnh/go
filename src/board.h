#pragma once

#include <vector>
#include <cstdint>
#include <string>

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

  struct Move { int x; int y; Stone s; bool pass; std::string comment; };
  const std::vector<Move>& moves() const { return moveHistory; }
  bool pass(Stone s);

  // helpers for SGF/metadata
  void appendMove(int x,int y, Stone s, bool pass=false, const std::string &comment = "");
  void setLastMoveComment(const std::string &c);

private:
  int N;
  std::vector<Stone> grid;
  bool hasLibertyDFS(int x,int y, std::vector<char>& visited) const;
  void removeGroup(int x,int y, Stone color);
  // Zobrist hashing & history for superko
  uint64_t currentHash{0};
  std::vector<uint64_t> hashHistory;
  Zobrist zobristTable{9};
  // Move history for SGF roundtrips
  std::vector<Move> moveHistory;
  void recordMove(int x,int y, Stone s, bool pass=false){ moveHistory.push_back({x,y,s,pass, std::string()}); }
  void recordPass(Stone s){ moveHistory.push_back({-1,-1,s,true, std::string()}); hashHistory.push_back(currentHash); }
};