#pragma once

#include "board.h"
#include "rules.h"

class Game {
public:
  explicit Game(int boardSize = 9, Ruleset rules = Ruleset::Chinese, double komi = 6.5);
  Stone currentPlayer() const;
  bool play(int x,int y); // plays move for current player if legal
  bool pass(); // current player passes
  void resign();
  bool isOver() const;
  Stone winner() const; // BLACK/WHITE or EMPTY if undecided
  const Board& board() const { return b; }

private:
  Board b;
  Stone toMove;
  int consecutivePasses{0};
  bool resigned{false};
  Stone winnerCached{EMPTY};
  Ruleset ruleset;
  double komi;
  void finalizeIfNeeded();
};
