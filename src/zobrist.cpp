#include "zobrist.h"
#include <random>
#include <array>

Zobrist::Zobrist(int N): N(N), table(N*N) {
  std::mt19937_64 rng(0x9e3779b97f4a7c15ULL); // deterministic seed for tests
  for(auto &e : table){
    e[0] = rng(); // BLACK
    e[1] = rng(); // WHITE
  }
}

uint64_t Zobrist::hash(const std::vector<Stone>& grid) const {
  uint64_t h = 0;
  for(int i=0;i<N*N;i++){
    if(grid[i]==BLACK) h ^= table[i][0];
    else if(grid[i]==WHITE) h ^= table[i][1];
  }
  return h;
}