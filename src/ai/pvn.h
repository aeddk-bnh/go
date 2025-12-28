#pragma once
#include <vector>
#include <memory>
#include "board.h"

// Policy/Value network interface. Implementations may be a real NN or a simple heuristic.
class PolicyValueNet {
public:
  virtual ~PolicyValueNet() = default;
  // returns a probability for each legal move (same order as legalMoves)
  virtual std::vector<double> policy(const Board& b, const std::vector<Board::Move>& legal) = 0;
  // returns value in [0,1] from BLACK perspective
  virtual double value(const Board& b) = 0;
};

// Simple heuristic PV: uses the same move_prior_score heuristic as fallback
std::shared_ptr<PolicyValueNet> makeSimpleHeuristicPV();
