#include "pvn.h"
#include <cmath>
#include <algorithm>

// Reuse a local heuristics similar to mcts move_prior_score
static double _move_prior_score_local(const Board& b, const Board::Move& mv){
  if(mv.pass) return 0.0;
  int N = b.size();
  double cx = (N-1)/2.0, cy = (N-1)/2.0;
  double dx = mv.x - cx, dy = mv.y - cy;
  double dist = std::sqrt(dx*dx + dy*dy);
  double center_score = static_cast<double>(N) - dist;
  int adj = 0;
  for(int dy2=-1; dy2<=1; ++dy2) for(int dx2=-1; dx2<=1; ++dx2){
    if(dx2==0 && dy2==0) continue;
    int nx = mv.x + dx2, ny = mv.y + dy2;
    if(nx>=0 && ny>=0 && nx<N && ny<N){ if(b.get(nx,ny)!=EMPTY) adj++; }
  }
  return center_score + 2.0*adj;
}

class SimpleHeuristicPV : public PolicyValueNet {
public:
  std::vector<double> policy(const Board& b, const std::vector<Board::Move>& legal) override {
    std::vector<double> out; out.reserve(legal.size()); double tot=0.0;
    for(const auto &m: legal){ double s=_move_prior_score_local(b,m)+1.0; out.push_back(s); tot+=s; }
    if(tot<=0){ for(size_t i=0;i<out.size();++i) out[i]=1.0/out.size(); }
    else std::transform(out.begin(), out.end(), out.begin(), [tot](double v){ return v / tot; });
    return out;
  }
  double value(const Board& b) override {
    // simple neutral value
    return 0.5;
  }
};

std::shared_ptr<PolicyValueNet> makeSimpleHeuristicPV(){
  return std::make_shared<SimpleHeuristicPV>();
}
