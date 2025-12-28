#pragma once

#include <string>
#include "board.h"

namespace SGF {
  // Parse SGF content into board; returns true on success. Outputs komi if present.
  bool parse(const std::string& sgf, Board& out, double& komi_out);
  // Write a minimal SGF string from the board's move history
  std::string write(const Board& b, double komi=0.0);
}
