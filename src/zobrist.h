#pragma once

#include <vector>
#include <cstdint>
#include <array>

#include "types.h" // for Stone

class Zobrist {
public:
  Zobrist(int N);
  uint64_t hash(const std::vector<Stone>& grid) const;
private:
  int N;
  // table[pos][colorIndex] where colorIndex: 0=BLACK,1=WHITE
  std::vector<std::array<uint64_t,2>> table;
};