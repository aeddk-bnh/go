#pragma once

#include <utility>
#include "board.h"

enum class Ruleset { Japanese, Chinese };

class Scorer {
public:
  // Returns pair {black_score, white_score (includes komi)}
  static std::pair<double,double> score(const Board& b, Ruleset r, double komi = 6.5);
};