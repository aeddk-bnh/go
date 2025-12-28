#include "game.h"
#include "rules.h"

Game::Game(int boardSize, Ruleset rules, double komi): b(boardSize), toMove(BLACK), ruleset(rules), komi(komi) {}

Stone Game::currentPlayer() const { return toMove; }

bool Game::play(int x,int y){
  if(resigned || isOver()) return false;
  if(!b.isLegal(x,y,toMove)) return false;
  if(!b.place(x,y,toMove)) return false;
  consecutivePasses = 0;
  toMove = (toMove==BLACK?WHITE:BLACK);
  return true;
}

bool Game::pass(){
  if(resigned || isOver()) return false;
  b.pass(toMove);
  consecutivePasses++;
  if(consecutivePasses>=2) finalizeIfNeeded();
  toMove = (toMove==BLACK?WHITE:BLACK);
  return true;
}

void Game::resign(){
  if(resigned || isOver()) return;
  resigned = true;
  // The player who just played resigns => the current player (toMove) is the winner
  winnerCached = toMove;
}

bool Game::isOver() const {
  return resigned || (consecutivePasses>=2) || (winnerCached!=EMPTY);
}

Stone Game::winner() const {
  if(winnerCached!=EMPTY) return winnerCached;
  return EMPTY;
}

void Game::finalizeIfNeeded(){
  if(resigned || winnerCached!=EMPTY) return;
  auto sc = Scorer::score(b, ruleset, komi);
  double black = sc.first;
  double white = sc.second;
  if(black>white) winnerCached = BLACK;
  else winnerCached = WHITE;
}
