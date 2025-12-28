#include "mcts.h"
#include "rules.h"
#include "thread_pool.h"
#include <cmath>
#include <algorithm>
#include <mutex>
#include <atomic>

// Simple heuristic prior: prefer center and moves adjacent to existing stones
static double move_prior_score(const Board& b, const Board::Move& mv){
  if(mv.pass) return 0.0;
  int N = b.size();
  double cx = (N-1)/2.0, cy = (N-1)/2.0;
  double dx = mv.x - cx, dy = mv.y - cy;
  double dist = std::sqrt(dx*dx + dy*dy);
  double center_score = (double)(N) - dist; // closer to center -> higher
  // adjacency bonus
  int adj = 0;
  for(int dy2=-1; dy2<=1; ++dy2) for(int dx2=-1; dx2<=1; ++dx2){
    if(dx2==0 && dy2==0) continue;
    int nx = mv.x + dx2, ny = mv.y + dy2;
    if(nx>=0 && ny>=0 && nx<N && ny<N){ if(b.get(nx,ny)!=EMPTY) adj++; }
  }
  double adj_score = adj;
  return center_score + 2.0*adj_score;
}

MCTS::MCTS(const MCTSConfig& cfg): cfg(cfg), rng(0xC0FFEE) {}

std::vector<Board::Move> MCTS::legalMoves(const Board& b, Stone toPlay){
  std::vector<Board::Move> moves;
  int N = b.size();
  for(int y=0;y<N;y++) for(int x=0;x<N;x++){
    if(b.get(x,y)==EMPTY){ moves.push_back({x,y,toPlay,false, std::string()}); }
  }
  // pass as legal move
  moves.push_back({-1,-1,toPlay,true, std::string()});
  return moves;
}

double MCTS::rollout(Board state, Stone player){
  // play random moves until both pass consecutively or depth
  Stone cur = player;
  int passes = 0;
  for(int d=0; d<cfg.playout_depth; ++d){
    auto moves = legalMoves(state, cur);
    // Weighted random rollout preferring priors
    std::vector<double> w(moves.size()); double total=0.0;
    for(size_t i=0;i<moves.size();++i){ w[i] = move_prior_score(state, moves[i]) + 1.0; total += w[i]; }
    std::uniform_real_distribution<double> ud(0.0, total);
    double r = ud(rng);
    size_t idx=0; double acc=0.0; for(; idx<moves.size(); ++idx){ acc+=w[idx]; if(r<=acc) break; }
    if(idx>=moves.size()) idx = moves.size()-1;
    auto m = moves[idx];
    if(m.pass){ state.pass(cur); passes++; }
    else { state.place(m.x,m.y,cur); passes = 0; }
    if(passes>=2) break;
    cur = (cur==BLACK?WHITE:BLACK);
  }
  auto sc = Scorer::score(state, Ruleset::Chinese, 6.5);
  // return 1 if BLACK wins, 0 if WHITE wins (from perspective of BLACK)
  return sc.first > sc.second ? 1.0 : 0.0;
}

MCTS::Node* MCTS::select(Node* node){
  while(true){
    // If this node still has untried moves, return it for expansion
    if(!node->untriedMoves.empty()) return node;
    // If there are no children to descend into, return this node
    if(node->children.empty()) return node;
    // UCT selection (account for virtual loss)
    double best = -1e100; int bi = -1;
    for(size_t i=0;i<node->children.size();++i){
      Node* c = node->children[i].get();
      int cvis = c->visits.load();
      int cvl = c->virtual_loss.load();
      double Q = (cvis==0)?0.0:(c->value / (double)cvis);
        double denom = (double)(cvis + cvl + 1);
        double U = cfg.exploration * std::sqrt(std::log((double)node->visits.load()+1.0) / denom);
        double p = move_prior_score(node->state, c->moveFromParent);
        double prior_term = cfg.prior_weight * p / (1.0 + (double)cvis);
        double score = Q + U + prior_term;
      if(score > best){ best = score; bi = (int)i; }
    }
    node = node->children[bi].get();
  }
}

MCTS::Node* MCTS::expand(Node* node){
  // Progressive widening: limit number of children allowed based on node visits
  double visits = (double)node->visits.load();
  size_t max_children = std::max<size_t>(1, (size_t)(cfg.pw_k * std::pow(visits+1.0, cfg.pw_alpha)));
  if(node->untriedMoves.empty() || node->children.size() >= max_children) return node;
  // choose untried move weighted by prior
  std::vector<double> weights(node->untriedMoves.size()); double tot=0.0;
  for(size_t i=0;i<node->untriedMoves.size();++i){ weights[i]=move_prior_score(node->state, node->untriedMoves[i])+1.0; tot+=weights[i]; }
  std::uniform_real_distribution<double> ud(0.0, tot);
  double r = ud(rng); size_t idx=0; double acc=0.0; for(; idx<weights.size(); ++idx){ acc+=weights[idx]; if(r<=acc) break; }
  if(idx>=node->untriedMoves.size()) idx = node->untriedMoves.size()-1;
  auto mv = node->untriedMoves[idx];
  // remove move from untried
  node->untriedMoves.erase(node->untriedMoves.begin()+idx);
  // create child state
  Board childState = node->state;
  if(mv.pass) childState.pass(mv.s);
  else childState.place(mv.x,mv.y,mv.s);
  Stone next = (mv.s==BLACK?WHITE:BLACK);
  auto child = std::make_unique<Node>(childState, next, mv);
  child->untriedMoves = legalMoves(childState, next);
  child->parent = node;
  Node* ptr = child.get();
  node->children.push_back(std::move(child));
  // register in transposition table
  tt.insert(ptr->state.zobrist(), (void*)ptr);
  return ptr;
}

void MCTS::backpropagate(Node* node, double result){
  // result is from BLACK perspective
  while(node!=nullptr){
    // remove any virtual loss reserved on this node (if any)
    int vl = node->virtual_loss.load();
    if(vl > 0) node->virtual_loss.fetch_sub(1);
    node->visits.fetch_add(1);
    {
      std::lock_guard<std::mutex> lk(node->node_mutex);
      node->value += result;
    }
    node = node->parent;
  }
}

Board::Move MCTS::run(const Board& root, Stone toPlay){
  // single-threaded wrapper
  return runParallel(root, toPlay, cfg.iterations, 1);
}

Board::Move MCTS::runParallel(const Board& root, Stone toPlay, int iterations, int nThreads){
  rootNode = std::make_unique<Node>(root, toPlay, Board::Move{-1,-1,toPlay,true,std::string()});
  rootNode->untriedMoves = legalMoves(root, toPlay);
  // clear transposition table and register root
  tt.clear();
  tt.insert(rootNode->state.zobrist(), (void*)rootNode.get());

  std::atomic<int> remaining(iterations);
  ThreadPool pool((size_t)std::max(1, nThreads));

  auto worker = [this, &remaining]() {
    // seed a thread-local RNG from global rng once
    static thread_local bool seeded = false;
    static thread_local std::mt19937_64 local_rng;
    if(!seeded){ uint64_t s; { std::lock_guard<std::mutex> rlk(this->rng_mutex); s = this->rng(); } local_rng.seed(s); seeded = true; }

    while (true) {
      int it = remaining.fetch_sub(1, std::memory_order_relaxed);
      if (it <= 0) break;

      // Selection with virtual loss reservation
      Node* node = rootNode.get();
      std::vector<Node*> path;
      path.push_back(node);

      while (true) {
        // lock node to read children/untriedMoves consistently
        std::unique_lock<std::mutex> lk(node->node_mutex);
        // progressive widening: allow expansion only if children < threshold
        double visits_local = (double)node->visits.load();
        size_t max_children_local = std::max<size_t>(1, (size_t)(cfg.pw_k * std::pow(visits_local+1.0, cfg.pw_alpha)));
        if (!node->untriedMoves.empty() && node->children.size() < max_children_local) { lk.unlock(); break; }
        // If there are no children to descend into, expand here
        if (node->children.empty()) { lk.unlock(); break; }
        double best = -1e100; int bi = -1;
        for (size_t i = 0; i < node->children.size(); ++i) {
          Node* c = node->children[i].get();
          int cvis = c->visits.load();
          int cvl = c->virtual_loss.load();
          double Q = (cvis==0) ? 0.0 : (c->value / (double)cvis);
          double denom = (double)(cvis + cvl + 1);
          double U = cfg.exploration * std::sqrt(std::log((double)node->visits.load() + 1.0) / denom);
          double p = move_prior_score(node->state, c->moveFromParent);
          double prior_term = cfg.prior_weight * p / (1.0 + (double)cvis);
          double score = Q + U + prior_term;
          if (score > best) { best = score; bi = (int)i; }
        }
        Node* chosen = node->children[bi].get();
        // reserve virtual loss on chosen and move down
        chosen->virtual_loss.fetch_add(1);
        lk.unlock();
        node = chosen;
        path.push_back(node);
      }

      // Expansion
      Node* leaf = node;
      {
        std::unique_lock<std::mutex> lk(leaf->node_mutex);
        if (!leaf->untriedMoves.empty()) {
          // pick random untried move weighted by prior
          std::vector<double> weights(leaf->untriedMoves.size()); double tot=0.0;
          for(size_t i=0;i<leaf->untriedMoves.size();++i){ weights[i]=move_prior_score(leaf->state, leaf->untriedMoves[i])+1.0; tot+=weights[i]; }
          std::uniform_real_distribution<double> ud(0.0, tot);
          double rr = ud(local_rng); size_t idx=0; double acc=0.0; for(; idx<weights.size(); ++idx){ acc+=weights[idx]; if(rr<=acc) break; }
          if(idx>=leaf->untriedMoves.size()) idx = leaf->untriedMoves.size()-1;
          auto mv = leaf->untriedMoves[idx];
          leaf->untriedMoves.erase(leaf->untriedMoves.begin() + idx);
          Board childState = leaf->state;
          if (mv.pass) childState.pass(mv.s);
          else childState.place(mv.x, mv.y, mv.s);
          Stone next = (mv.s == BLACK ? WHITE : BLACK);
          auto child = std::make_unique<Node>(childState, next, mv);
          child->untriedMoves = legalMoves(childState, next);
          child->parent = leaf;
          Node* ptr = child.get();
          leaf->children.push_back(std::move(child));
          // register in transposition table
          {
            tt.insert(ptr->state.zobrist(), (void*)ptr);
          }
          // reserve virtual loss on new child
          ptr->virtual_loss.fetch_add(1);
          leaf = ptr;
          path.push_back(leaf);
        }
      }

      // Simulation
      double z = rollout(leaf->state, leaf->playerToMove);

      // Backpropagate and remove virtual losses
      for (auto itn = path.rbegin(); itn != path.rend(); ++itn) {
        Node* n = *itn;
        // remove virtual loss
        n->virtual_loss.fetch_sub(1);
        // update stats (protect value)
        {
          std::lock_guard<std::mutex> lk(n->node_mutex);
          n->visits.fetch_add(1);
          n->value += z;
        }
      }
    }
  };

  // start workers
  for (int i = 0; i < nThreads; ++i) pool.enqueue(worker);
  // destructor of pool will join when leaving scope; ensure all tasks are enqueued
  // wait until tasks finish by letting pool go out of scope
  // (pool will be destroyed at end of function)

  // give pool destructor a chance by doing a small idle wait; not strictly necessary
  // choose best by visits
  Node* best = nullptr; int bestVisits = -1;
  for (auto &c : rootNode->children) { int v = c->visits.load(); if (v > bestVisits) { bestVisits = v; best = c.get(); } }
  if (best) return best->moveFromParent;
  return Board::Move{-1,-1,toPlay,true,std::string()};
}