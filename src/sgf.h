#pragma once

#include <string>
#include "board.h"

namespace SGF {
  struct Game {
    std::string PB;
    std::string PW;
    std::string RE;
    double KM = 0.0;
    std::vector<Board::Move> moves;
  };

  // Parse SGF content into board; returns true on success. If 'game' is provided it will be filled with metadata and moves.
  bool parse(const std::string& sgf, Board& out, double& komi_out, Game* game=nullptr);
  // Write a minimal SGF string from the board's move history (deprecated) or from a Game
  std::string write(const Board& b, double komi=0.0);
  std::string write(const Game& g);

  // File helpers
  bool readFile(const std::string& path, Board& out, double& komi_out, Game* game=nullptr);
  bool writeFile(const std::string& path, const Game& g);
}
