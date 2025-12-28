#include "ai/mcts.h"
#include <algorithm>

bool MCTS::moveToChild(const Board::Move &mv){
  if(!rootNode) return false;
  // Try transposition table lookup first: apply move to a temp board and look up its zobrist hash
  Board tmp = rootNode->state;
  if(mv.pass) tmp.pass(mv.s);
  else tmp.place(mv.x, mv.y, mv.s);
  {
    uint64_t h = tmp.zobrist();
    void* v = tt.get(h);
    if(v){
      const Node* found = static_cast<Node*>(v);
      // ensure found is a direct child of rootNode
      for(size_t i=0;i<rootNode->children.size();++i){
        if(rootNode->children[i].get() == found){
          auto newRoot = std::move(rootNode->children[i]);
          rootNode->children.erase(rootNode->children.begin()+i);
          newRoot->parent = nullptr;
          rootNode = std::move(newRoot);
          return true;
        }
      }
    }
  }

  // Fallback: match by move fields (older behavior)
  for(size_t i=0;i<rootNode->children.size();++i){
    const auto &cptr = rootNode->children[i];
    if(cptr->moveFromParent.x==mv.x && cptr->moveFromParent.y==mv.y && cptr->moveFromParent.pass==mv.pass && cptr->moveFromParent.s==mv.s){
      auto newRoot = std::move(rootNode->children[i]);
      rootNode->children.erase(rootNode->children.begin()+i);
      newRoot->parent = nullptr;
      rootNode = std::move(newRoot);
      return true;
    }
  }
  // If move was not found among expanded children, check untriedMoves at root and expand that move into a new root
  for(size_t i=0;i<rootNode->untriedMoves.size();++i){
    auto um = rootNode->untriedMoves[i];
    if(um.x==mv.x && um.y==mv.y && um.pass==mv.pass && um.s==mv.s){
      // create new child node for this move
      Board childState = rootNode->state;
      if(um.pass) childState.pass(um.s);
      else childState.place(um.x, um.y, um.s);
      Stone next = (um.s==BLACK?WHITE:BLACK);
      auto child = std::make_unique<Node>(childState, next, um);
      child->untriedMoves = legalMoves(childState, next);
      child->parent = nullptr;
      // remove untried move from root
      rootNode->untriedMoves.erase(rootNode->untriedMoves.begin()+i);
      rootNode = std::move(child);
      // register new root in transposition table
      tt.insert(rootNode->state.zobrist(), static_cast<void*>(rootNode.get()));
      return true;
    }
  }
  return false;
}

bool MCTS::applyVirtualLossToChildIndex(size_t idx, int loss){
  if(!rootNode || idx>=rootNode->children.size()) return false;
  rootNode->children[idx]->virtual_loss.fetch_add(loss);
  return true;
}

bool MCTS::revertVirtualLossFromChildIndex(size_t idx, int loss){
  if(!rootNode || idx>=rootNode->children.size()) return false;
  int prev = rootNode->children[idx]->virtual_loss.fetch_sub(loss);
  if(prev - loss < 0) rootNode->children[idx]->virtual_loss.store(0);
  return true;
}

int MCTS::chooseChildIndexAtRoot() const{
  if(!rootNode) return -1;
  double best = -1e100; int bi = -1;
  for(size_t i=0;i<rootNode->children.size();++i){
    auto c = rootNode->children[i].get();
    int cvis = c->visits.load();
    int cvl = c->virtual_loss.load();
    double Q = (cvis==0)?0.0:(c->value / static_cast<double>(cvis));
    // Amplify virtual-loss effect so it meaningfully penalizes selection at root
    double denom = static_cast<double>(cvis + 1 + cvl * 10);
    double U = cfg.exploration * std::sqrt(std::log1p(static_cast<double>(rootNode->visits.load())) / denom);
    // penalize by virtual loss to bias selection away from nodes under simulation
    double score = Q + U - static_cast<double>(cvl) * 1.0;
    (void)0;
    if(score > best){ best = score; bi = (int)i; }
  }
  return bi;
}