#pragma once

#include <vector>
#include <memory>
#include <random>
#include "board.h"

struct MCTSConfig {
  int iterations = 1000;
  int playout_depth = 200; // safety
  double exploration = 1.4;
};

class MCTS {
public:
  MCTS(const MCTSConfig& cfg = MCTSConfig());
  // returns best move as (x,y), with x=-1,y=-1 meaning pass
  Board::Move run(const Board& root, Stone toPlay);
private:
  MCTSConfig cfg;
  std::mt19937_64 rng;

  struct Node {
    Board state;
    Stone playerToMove; // player who will play at this node
    Board::Move moveFromParent; // move that led to this node
    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;
    int visits = 0;
    double value = 0.0; // total value from perspective of root player
    std::vector<Board::Move> untriedMoves;

    Node(const Board& s, Stone p, const Board::Move& mv) : state(s), playerToMove(p), moveFromParent(mv) {}
  };

  std::unique_ptr<Node> rootNode;

  std::vector<Board::Move> legalMoves(const Board& b, Stone toPlay);
  double rollout(Board state, Stone player);
  Node* select(Node* node);
  Node* expand(Node* node);
  void backpropagate(Node* node, double result);
};