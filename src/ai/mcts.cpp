#include "mcts.h"
#include "rules.h"
#include <cmath>
#include <algorithm>

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
    std::uniform_int_distribution<size_t> dist(0, moves.size()-1);
    auto m = moves[dist(rng)];
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
  while(!node->children.empty()){
    // UCT selection
    double best = -1e100; int bi = -1;
    for(size_t i=0;i<node->children.size();++i){
      Node* c = node->children[i].get();
      double Q = (c->visits==0)?0.0:(c->value / c->visits);
      double U = cfg.exploration * std::sqrt(std::log(node->visits+1) / (c->visits+1));
      double score = Q + U;
      if(score > best){ best = score; bi = (int)i; }
    }
    node = node->children[bi].get();
  }
  return node;
}

MCTS::Node* MCTS::expand(Node* node){
  if(node->untriedMoves.empty()) return node;
  std::uniform_int_distribution<size_t> dist(0, node->untriedMoves.size()-1);
  size_t idx = dist(rng);
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
  Node* ptr = child.get();
  node->children.push_back(std::move(child));
  return ptr;
}

void MCTS::backpropagate(Node* node, double result){
  // result is from BLACK perspective
  while(node!=nullptr){
    node->visits += 1;
    // value accumulation: if node->playerToMove is BLACK, prefer result, else (WHITE) prefer 1-result
    node->value += result;
    node = node->parent;
  }
}

Board::Move MCTS::run(const Board& root, Stone toPlay){
  rootNode = std::make_unique<Node>(root, toPlay, Board::Move{-1,-1,toPlay,true,std::string()});
  rootNode->untriedMoves = legalMoves(root, toPlay);
  for(int it=0; it<cfg.iterations; ++it){
    Node* node = select(rootNode.get());
    if(!node->untriedMoves.empty()){
      Node* leaf = expand(node);
      // simulate from leaf
      double z = rollout(leaf->state, leaf->playerToMove);
      backpropagate(leaf, z);
    } else {
      // leaf is terminal or fully expanded; simulate directly
      double z = rollout(node->state, node->playerToMove);
      backpropagate(node, z);
    }
  }
  // choose best by visits
  Node* best = nullptr; int bestVisits = -1;
  for(auto &c : rootNode->children){ if(c->visits > bestVisits){ bestVisits = c->visits; best = c.get(); } }
  if(best) return best->moveFromParent;
  // fallback: pass
  return Board::Move{-1,-1,toPlay,true,std::string()};
}