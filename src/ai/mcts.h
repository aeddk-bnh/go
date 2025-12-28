#pragma once

#include <vector>
#include <memory>
#include <random>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "board.h"
#include "tt_sharded.h"
#include "pvn.h"

struct MCTSConfig {
  int iterations = 1000;
  int playout_depth = 200; // safety
  double exploration = 1.4;
  // Heuristics
  double prior_weight = 0.5; // weight of move prior in selection
  double pw_alpha = 0.5; // progressive widening exponent
  double pw_k = 1.0; // progressive widening multiplier
};

class MCTS {
public:
  MCTS(const MCTSConfig& cfg = MCTSConfig());
  // returns best move as (x,y), with x=-1,y=-1 meaning pass
  Board::Move run(const Board& root, Stone toPlay);
private:
  MCTSConfig cfg;
  std::mt19937_64 rng;
  std::mutex rng_mutex; // used to seed per-worker RNGs safely

  struct Node {
    Board state;
    Stone playerToMove; // player who will play at this node
    Board::Move moveFromParent; // move that led to this node
    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;
    std::atomic<int> visits{0};
    double value = 0.0; // total value from perspective of root player
    std::atomic<int> virtual_loss{0}; // for multi-threading
    std::vector<Board::Move> untriedMoves;
    std::mutex node_mutex; // protects `value` and children modification

    Node(const Board& s, Stone p, const Board::Move& mv) : state(s), playerToMove(p), moveFromParent(mv) {}
  };

public:
  // choose child index at root using UCT (for testing/selection heuristics)
  int chooseChildIndexAtRoot() const;
  std::unique_ptr<Node> rootNode;
  // sharded transposition table (stores void* to Node)
  ShardedTT tt{64};

  // Parallel search API
  Board::Move runParallel(const Board& root, Stone toPlay, int iterations, int nThreads);

  std::vector<Board::Move> legalMoves(const Board& b, Stone toPlay);
  double rollout(Board state, Stone player, std::mt19937_64 &rng);
  Node* select(Node* node);
  Node* expand(Node* node);
  void backpropagate(Node* node, double result);

  // New features
public:
  // After applying a move on the external board, call this to move root to the corresponding child (reuse tree)
  bool moveToChild(const Board::Move &mv);
  // Apply virtual loss to a child at root (for multi-threading); returns false if index invalid
  bool applyVirtualLossToChildIndex(size_t idx, int loss=1);
  bool revertVirtualLossFromChildIndex(size_t idx, int loss=1);
  // Debug helpers
  uint64_t rootHash() const { return rootNode ? rootNode->state.zobrist() : 0; }
  int rootChildrenCount() const { return rootNode ? (int)rootNode->children.size() : 0; }
  int childVirtualLoss(size_t idx) const { return rootNode && idx<rootNode->children.size() ? rootNode->children[idx]->virtual_loss.load() : -1; }
  int childVisits(size_t idx) const { return rootNode && idx<rootNode->children.size() ? rootNode->children[idx]->visits.load() : -1; }
  // Policy/Value network (optional). Defaults to a simple heuristic PV.
  std::shared_ptr<PolicyValueNet> pv;
  void setPV(std::shared_ptr<PolicyValueNet> p) { pv = std::move(p); }
};