#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <fstream>
#include <limits>
#include <algorithm>
#include <iomanip>

#include "board.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using std::cin;
using std::cout;
using std::endl;

static void clearScreen(){ cout << "\x1b[2J\x1b[H"; }

// Simple coordinate parser: accepts D4 or "4 5" or "4,5"
bool parseCoord(const std::string &in, int &outx, int &outy){
  std::string s; for(char c: in) if(!isspace((unsigned char)c)) s.push_back(c);
  if(s.empty()) return false;
  if(std::isalpha((unsigned char)s[0])){
    char c = std::toupper((unsigned char)s[0]);
    int col = c - 'A';
    std::string num = s.substr(1);
    if(num.empty()) return false;
    try{ int r = std::stoi(num); outx = col; outy = r-1; return true;}catch(...){return false;}
  }
  std::replace(s.begin(), s.end(), ',', ' ');
  std::istringstream iss(s);
  int a,b; if(iss >> a >> b){ outx = a-1; outy = b-1; return true; }
  return false;
}

// lastMove: pair(x,y) or (-1,-1) for none
void printBoard(const Board &b, std::pair<int,int> lastMove = {-1,-1}){
  clearScreen();
  int N = b.size();
  auto colLabel = [](int x)->char{
    // Skip 'I' as per common Go notation
    char c = char('A' + x);
    if(c >= 'I') c = char(c + 1);
    return c;
  };
  cout << "   ";
  for(int x=0;x<N;++x){ cout << ' ' << colLabel(x) << ' '; }
  cout << '\n';
  for(int y=0;y<N;++y){
    cout << std::setw(2) << (y+1) << ' ';
    for(int x=0;x<N;++x){
      Stone s = b.get(x,y);
      bool isLast = (lastMove.first==x && lastMove.second==y);
      const char *c = (s==BLACK)?"\u25CF":(s==WHITE)?"\u25CB":".";
      if(isLast){ cout << ' ' << '*' << ' '; }
      else { cout << ' ' << c << ' '; }
    }
    cout << '\n';
  }
}

// Random playout used by MCTS
static Stone simulatePlayout(Board sim, int &simCapB, int &simCapW, Stone simTurn, std::mt19937_64 &rng){
  int boardSize = sim.size();
  int maxPlayoutMoves = boardSize * boardSize * 4;
  int consecutivePasses = 0;
  int moves = 0;
  while(consecutivePasses < 2 && moves < maxPlayoutMoves){
    auto movesList = std::vector<std::pair<int,int>>();
    for(int y=0;y<boardSize;y++) for(int x=0;x<boardSize;x++) if(sim.isLegal(x,y,simTurn)) movesList.emplace_back(x,y);
    movesList.emplace_back(-1,-1); // pass
    if(movesList.empty()) { consecutivePasses = 2; break; }
    std::uniform_int_distribution<size_t> dist(0, movesList.size()-1);
    auto pick = movesList[dist(rng)];
    if(pick.first==-1){ consecutivePasses++; sim.pass(simTurn); simTurn = (simTurn==BLACK?WHITE:BLACK); }
    else{
      consecutivePasses = 0;
      int beforeEnemy = 0;
      for(int yy=0; yy<boardSize; ++yy) for(int xx=0; xx<boardSize; ++xx) if(sim.get(xx,yy) == (simTurn==BLACK?WHITE:BLACK)) ++beforeEnemy;
      bool ok = sim.place(pick.first,pick.second,simTurn);
      if(!ok){ consecutivePasses++; simTurn = (simTurn==BLACK?WHITE:BLACK); }
      else{
        int afterEnemy = 0;
        for(int yy=0; yy<boardSize; ++yy) for(int xx=0; xx<boardSize; ++xx) if(sim.get(xx,yy) == (simTurn==BLACK?WHITE:BLACK)) ++afterEnemy;
        int cap = beforeEnemy - afterEnemy;
        if(cap>0){ if(simTurn==BLACK) simCapB += cap; else simCapW += cap; }
        simTurn = (simTurn==BLACK?WHITE:BLACK);
      }
    }
    moves++;
  }
  int blackTerr=0, whiteTerr=0;
  // compute territory similar to other helpers (simple flood)
  int N = sim.size();
  std::vector<char> seen(N*N,0);
  const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
  for(int y=0;y<N;y++) for(int x=0;x<N;x++){
    int id = y*N + x;
    if(seen[id]) continue;
    if(sim.get(x,y) != EMPTY){ seen[id]=1; continue; }
    std::vector<int> stack; stack.push_back(id); seen[id]=1;
    bool adjB=false, adjW=false;
    int regionCount=0;
    while(!stack.empty()){
      int cur = stack.back(); stack.pop_back(); regionCount++;
      int cx = cur % N, cy = cur / N;
      for(int i=0;i<4;i++){
        int nx=cx+dx[i], ny=cy+dy[i];
        if(nx<0||ny<0||nx>=N||ny>=N) continue;
        int nid = ny*N+nx;
        Stone s = sim.get(nx,ny);
        if(s==EMPTY && !seen[nid]){ seen[nid]=1; stack.push_back(nid); }
        else if(s==BLACK) adjB=true;
        else if(s==WHITE) adjW=true;
      }
    }
    if(adjB && !adjW) blackTerr += regionCount;
    else if(adjW && !adjB) whiteTerr += regionCount;
  }
  int blackStones=0, whiteStones=0;
  for(int y=0;y<N;++y) for(int x=0;x<N;++x){ Stone s = sim.get(x,y); if(s==BLACK) ++blackStones; else if(s==WHITE) ++whiteStones; }
  int blackTotal = blackStones + simCapB + blackTerr;
  int whiteTotal = whiteStones + simCapW + whiteTerr;
  return (blackTotal > whiteTotal) ? BLACK : WHITE;
}

struct MCTSNode{
  MCTSNode* parent = nullptr;
  std::vector<std::unique_ptr<MCTSNode>> children;
  std::vector<std::pair<int,int>> untried;
  std::pair<int,int> move{-2,-2};
  Stone playerJustMoved = EMPTY;
  std::atomic<size_t> wins{0}, visits{0};
  std::atomic<int> vloss{0};
  std::mutex mutex;
  MCTSNode(MCTSNode* p, const std::vector<std::pair<int,int>>&u, std::pair<int,int> m, Stone pjm)
    : parent(p), untried(u), move(m), playerJustMoved(pjm){}
};

// Detach and return child matching move `mv` from rootPtr if present; leaves child->parent == nullptr
static std::unique_ptr<MCTSNode> detachChildByMove(std::unique_ptr<MCTSNode> &rootPtr, const std::pair<int,int> &mv){
  if(!rootPtr) return nullptr;
  for(size_t i=0;i<rootPtr->children.size(); ++i){
    if(rootPtr->children[i]->move == mv){
      auto child = std::move(rootPtr->children[i]);
      rootPtr->children.erase(rootPtr->children.begin()+i);
      child->parent = nullptr;
      return child;
    }
  }
  return nullptr;
}

// MCTS using an existing root (subtree reuse). If `rootPtr` is null, a new root is created.
static std::pair<int,int> mctsMove(const Board &rootBoard, int rootCapB, int rootCapW, Stone toMove, size_t iterations, double Cp, std::mt19937_64 &rng, std::unique_ptr<MCTSNode> &rootPtr){
  Stone rootPJM = (toMove==BLACK?WHITE:BLACK);
  int N = rootBoard.size();
  if(!rootPtr){
    std::vector<std::pair<int,int>> rootMoves;
    for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(rootBoard.isLegal(x,y,toMove)) rootMoves.emplace_back(x,y);
    rootMoves.emplace_back(-1,-1);
    rootPtr = std::make_unique<MCTSNode>(nullptr, rootMoves, std::pair<int,int>{-2,-2}, rootPJM);
  }

  for(size_t it=0; it<iterations; ++it){
    MCTSNode* node = rootPtr.get();
    Board sim = rootBoard;
    int simCapB = rootCapB, simCapW = rootCapW;
    Stone simTurn = toMove;

    // selection
    while(node->untried.empty() && !node->children.empty()){
      double bestUCT = -std::numeric_limits<double>::infinity();
      MCTSNode* best=nullptr;
      for(auto &cptr: node->children){
        MCTSNode* c = cptr.get();
        if(c->visits.load()==0){ best = c; break; }
        double wr = double(c->wins.load())/double(c->visits.load());
        double uct = wr + Cp * std::sqrt(std::log(double(node->visits.load()))/double(c->visits.load()));
        if(uct > bestUCT){ bestUCT = uct; best = c; }
      }
      if(!best) break;
      auto mv = best->move;
      if(mv.first != -1) sim.place(mv.first,mv.second, simTurn);
      else sim.pass(simTurn);
      simTurn = (simTurn==BLACK?WHITE:BLACK);
      node = best;
    }

    // expansion
    if(!node->untried.empty()){
      std::uniform_int_distribution<size_t> dist(0, node->untried.size()-1);
      size_t idx = dist(rng);
      auto mv = node->untried[idx];
      {
        std::lock_guard<std::mutex> lk(node->mutex);
        if(idx >= node->untried.size()) continue;
        mv = node->untried[idx];
        node->untried.erase(node->untried.begin()+idx);
      }
      if(mv.first != -1) sim.place(mv.first,mv.second, simTurn);
      else sim.pass(simTurn);
      Stone pjm = simTurn;
      simTurn = (simTurn==BLACK?WHITE:BLACK);
      std::vector<std::pair<int,int>> childMoves;
      for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(sim.isLegal(x,y,simTurn)) childMoves.emplace_back(x,y);
      childMoves.emplace_back(-1,-1);
      {
        std::lock_guard<std::mutex> lk(node->mutex);
        node->children.emplace_back(std::make_unique<MCTSNode>(node, childMoves, mv, pjm));
        node = node->children.back().get();
      }
    }

    // simulation
    int simCapB_copy = simCapB, simCapW_copy = simCapW;
    Stone winner = simulatePlayout(sim, simCapB_copy, simCapW_copy, simTurn, rng);

    // backprop: add real visit/win (single-threaded variant)
    MCTSNode* back = node;
    while(back!=nullptr){ back->visits.fetch_add(1); if(back->playerJustMoved == winner) back->wins.fetch_add(1); back = back->parent; }
  }

  // choose most visited child
  MCTSNode* best=nullptr; size_t bestV=0;
  for(auto &cptr: rootPtr->children) { size_t v = cptr->visits.load(); if(v > bestV){ best = cptr.get(); bestV = v; } }
  if(!best) return {-1,-1};
  return best->move;
}

// Time-limited MCTS: run iterations until `seconds` elapsed (approx).
// Time-limited MCTS that can reuse an existing root passed by reference.
static std::pair<int,int> mctsMoveTimed(const Board &rootBoard, int rootCapB, int rootCapW, Stone toMove, double seconds, double Cp, std::mt19937_64 &rng, std::unique_ptr<MCTSNode> &rootPtr){
  using clock = std::chrono::steady_clock;
  auto deadline = clock::now() + std::chrono::duration<double>(seconds);

  Stone rootPJM = (toMove==BLACK?WHITE:BLACK);
  int N = rootBoard.size();
  if(!rootPtr){
    std::vector<std::pair<int,int>> rootMoves;
    for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(rootBoard.isLegal(x,y,toMove)) rootMoves.emplace_back(x,y);
    rootMoves.emplace_back(-1,-1);
    rootPtr = std::make_unique<MCTSNode>(nullptr, rootMoves, std::pair<int,int>{-2,-2}, rootPJM);
  }

  while(clock::now() < deadline){
    MCTSNode* node = rootPtr.get();
    Board sim = rootBoard;
    int simCapB = rootCapB, simCapW = rootCapW;
    Stone simTurn = toMove;

    // apply virtual loss to root
    node->vloss.fetch_add(1);
    std::vector<MCTSNode*> path;
    path.push_back(node);

    // selection (copy children under lock, then evaluate)
    while(true){
      std::vector<MCTSNode*> children_copy;
      {
        std::lock_guard<std::mutex> lk(node->mutex);
        if(!node->untried.empty()) break;
        if(node->children.empty()) break;
        for(auto &cptr: node->children) children_copy.push_back(cptr.get());
      }
      if(children_copy.empty()) break;
      double bestUCT = -std::numeric_limits<double>::infinity();
      MCTSNode* best = nullptr;
      for(MCTSNode* c: children_copy){
        // use visits + vloss to account for ongoing simulations
        int denom = int(c->visits.load()) + c->vloss.load();
        if(denom == 0){ best = c; break; }
        double wr = double(c->wins.load()) / double(denom);
        double uct = wr + Cp * std::sqrt(std::log(double(node->visits.load() + node->vloss.load()))/double(denom));
        if(uct > bestUCT){ bestUCT = uct; best = c; }
      }
      if(!best) break;
      auto mv = best->move;
      // mark virtual loss for child and add to path
      best->vloss.fetch_add(1);
      path.push_back(best);
      if(mv.first != -1) sim.place(mv.first,mv.second, simTurn);
      else sim.pass(simTurn);
      simTurn = (simTurn==BLACK?WHITE:BLACK);
      node = best;
    }

    // expansion
    {
      std::lock_guard<std::mutex> lk(node->mutex);
      if(!node->untried.empty()){
        std::uniform_int_distribution<size_t> dist(0, node->untried.size()-1);
        size_t idx = dist(rng);
        if(idx < node->untried.size()){
          auto mv = node->untried[idx];
          node->untried.erase(node->untried.begin()+idx);
          if(mv.first != -1) sim.place(mv.first,mv.second, simTurn);
          else sim.pass(simTurn);
          Stone pjm = simTurn;
          simTurn = (simTurn==BLACK?WHITE:BLACK);
          std::vector<std::pair<int,int>> childMoves;
          for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(sim.isLegal(x,y,simTurn)) childMoves.emplace_back(x,y);
          childMoves.emplace_back(-1,-1);
          node->children.emplace_back(std::make_unique<MCTSNode>(node, childMoves, mv, pjm));
          node = node->children.back().get();
          // add new node to path and set its virtual loss
          node->vloss.fetch_add(1);
          path.push_back(node);
        }
      }
    }

    // simulation
    int simCapB_copy = simCapB, simCapW_copy = simCapW;
    Stone winner = simulatePlayout(sim, simCapB_copy, simCapW_copy, simTurn, rng);

    // backprop
    MCTSNode* back = node;
    while(back!=nullptr){ back->visits.fetch_add(1); if(back->playerJustMoved == winner) back->wins.fetch_add(1); back = back->parent; }
  }

  // choose most visited child
  MCTSNode* best=nullptr; size_t bestV=0;
  for(auto &cptr: rootPtr->children) {
    size_t v = cptr->visits.load();
    if(v > bestV){ best = cptr.get(); bestV = v; }
  }
  if(!best) return {-1,-1};
  return best->move;
}

// Parallel MCTS (iterations distributed across worker threads) with virtual loss.
static std::pair<int,int> mctsParallel(const Board &rootBoard, int rootCapB, int rootCapW, Stone toMove, size_t iterations, int numThreads, double Cp, std::unique_ptr<MCTSNode> &rootPtr){
  int N = rootBoard.size();
  if(numThreads <= 1) {
    std::mt19937_64 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return mctsMove(rootBoard, rootCapB, rootCapW, toMove, iterations, Cp, rng, rootPtr);
  }
  std::atomic<size_t> remaining(iterations);
  std::vector<std::thread> workers;
  std::mutex seedMutex;
  std::mt19937_64 seedRng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());

  auto worker = [&](int id){
    // per-thread RNG
    uint64_t seed;
    { std::lock_guard<std::mutex> lk(seedMutex); seed = seedRng(); }
    std::mt19937_64 rng_local(seed ^ (uint64_t(id) + 0x9e3779b97f4a7c15ULL));
    while(true){
      size_t prev = remaining.fetch_sub(1);
      if(prev==0) break;

      // single iteration similar to timed variant
      MCTSNode* node = rootPtr.get();
      Board sim = rootBoard;
      int simCapB = rootCapB, simCapW = rootCapW;
      Stone simTurn = toMove;
      // apply virtual loss to root
      node->vloss.fetch_add(1);
      std::vector<MCTSNode*> path;
      path.push_back(node);

      // selection
      while(true){
        std::vector<MCTSNode*> children_copy;
        {
          std::lock_guard<std::mutex> lk(node->mutex);
          if(!node->untried.empty()) break;
          if(node->children.empty()) break;
          for(auto &cptr: node->children) children_copy.push_back(cptr.get());
        }
        if(children_copy.empty()) break;
        double bestUCT = -std::numeric_limits<double>::infinity();
        MCTSNode* best = nullptr;
        for(MCTSNode* c: children_copy){
          int denom = int(c->visits.load()) + c->vloss.load();
          if(denom == 0){ best = c; break; }
          double wr = double(c->wins.load()) / double(denom);
          double uct = wr + Cp * std::sqrt(std::log(double(node->visits.load() + node->vloss.load()))/double(denom));
          if(uct > bestUCT){ bestUCT = uct; best = c; }
        }
        if(!best) break;
        auto mv = best->move;
        best->vloss.fetch_add(1);
        path.push_back(best);
        if(mv.first != -1) sim.place(mv.first,mv.second, simTurn);
        else sim.pass(simTurn);
        simTurn = (simTurn==BLACK?WHITE:BLACK);
        node = best;
      }

      // expansion
      {
        std::lock_guard<std::mutex> lk(node->mutex);
        if(!node->untried.empty()){
          std::uniform_int_distribution<size_t> dist(0, node->untried.size()-1);
          size_t idx = dist(rng_local);
          if(idx < node->untried.size()){
            auto mv = node->untried[idx];
            node->untried.erase(node->untried.begin()+idx);
            if(mv.first != -1) sim.place(mv.first,mv.second, simTurn);
            else sim.pass(simTurn);
            Stone pjm = simTurn;
            simTurn = (simTurn==BLACK?WHITE:BLACK);
            std::vector<std::pair<int,int>> childMoves;
            for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(sim.isLegal(x,y,simTurn)) childMoves.emplace_back(x,y);
            childMoves.emplace_back(-1,-1);
            node->children.emplace_back(std::make_unique<MCTSNode>(node, childMoves, mv, pjm));
            node = node->children.back().get();
            node->vloss.fetch_add(1);
            path.push_back(node);
          }
        }
      }

      // simulation
      int simCapB_copy = simCapB, simCapW_copy = simCapW;
      Stone winner = simulatePlayout(sim, simCapB_copy, simCapW_copy, simTurn, rng_local);

      // backprop
      for(MCTSNode* n : path){ n->vloss.fetch_sub(1); n->visits.fetch_add(1); if(n->playerJustMoved == winner) n->wins.fetch_add(1); }
    }
  };

  for(int i=0;i<numThreads;i++) workers.emplace_back(worker, i);
  for(auto &t: workers) t.join();

  // choose most visited child
  MCTSNode* best=nullptr; size_t bestV=0;
  for(auto &cptr: rootPtr->children) {
    size_t v = cptr->visits.load();
    if(v > bestV){ best = cptr.get(); bestV = v; }
  }
  if(!best) return {-1,-1};
  return best->move;
}

// Parallel time-limited MCTS using worker threads
static std::pair<int,int> mctsParallelTimed(const Board &rootBoard, int rootCapB, int rootCapW, Stone toMove, double seconds, int numThreads, double Cp, std::unique_ptr<MCTSNode> &rootPtr){
  using clock = std::chrono::steady_clock;
  auto deadline = clock::now() + std::chrono::duration<double>(seconds);
  int N = rootBoard.size();
  if(numThreads <= 1) {
    std::mt19937_64 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return mctsMoveTimed(rootBoard, rootCapB, rootCapW, toMove, seconds, Cp, rng, rootPtr);
  }
  std::vector<std::thread> workers;
  std::mutex seedMutex;
  std::mt19937_64 seedRng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
  auto worker = [&](int id){
    uint64_t seed;
    { std::lock_guard<std::mutex> lk(seedMutex); seed = seedRng(); }
    std::mt19937_64 rng_local(seed ^ (uint64_t(id) + 0x9e3779b97f4a7c15ULL));
    while(clock::now() < deadline){
      // single iteration
      MCTSNode* node = rootPtr.get();
      Board sim = rootBoard;
      int simCapB = rootCapB, simCapW = rootCapW;
      Stone simTurn = toMove;
      node->vloss.fetch_add(1);
      std::vector<MCTSNode*> path;
      path.push_back(node);

      while(true){
        std::vector<MCTSNode*> children_copy;
        {
          std::lock_guard<std::mutex> lk(node->mutex);
          if(!node->untried.empty()) break;
          if(node->children.empty()) break;
          for(auto &cptr: node->children) children_copy.push_back(cptr.get());
        }
        if(children_copy.empty()) break;
        double bestUCT = -std::numeric_limits<double>::infinity();
        MCTSNode* best = nullptr;
        for(MCTSNode* c: children_copy){
          int denom = int(c->visits.load()) + c->vloss.load();
          if(denom == 0){ best = c; break; }
          double wr = double(c->wins.load()) / double(denom);
          double uct = wr + Cp * std::sqrt(std::log(double(node->visits.load() + node->vloss.load()))/double(denom));
          if(uct > bestUCT){ bestUCT = uct; best = c; }
        }
        if(!best) break;
        auto mv = best->move;
        best->vloss.fetch_add(1);
        path.push_back(best);
        if(mv.first != -1) sim.place(mv.first,mv.second, simTurn);
        else sim.pass(simTurn);
        simTurn = (simTurn==BLACK?WHITE:BLACK);
        node = best;
      }

      {
        std::lock_guard<std::mutex> lk(node->mutex);
        if(!node->untried.empty()){
          std::uniform_int_distribution<size_t> dist(0, node->untried.size()-1);
          size_t idx = dist(rng_local);
          if(idx < node->untried.size()){
            auto mv = node->untried[idx];
            node->untried.erase(node->untried.begin()+idx);
            if(mv.first != -1) sim.place(mv.first,mv.second, simTurn);
            else sim.pass(simTurn);
            Stone pjm = simTurn;
            simTurn = (simTurn==BLACK?WHITE:BLACK);
            std::vector<std::pair<int,int>> childMoves;
            for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(sim.isLegal(x,y,simTurn)) childMoves.emplace_back(x,y);
            childMoves.emplace_back(-1,-1);
            node->children.emplace_back(std::make_unique<MCTSNode>(node, childMoves, mv, pjm));
            node = node->children.back().get();
            node->vloss.fetch_add(1);
            path.push_back(node);
          }
        }
      }

      int simCapB_copy = simCapB, simCapW_copy = simCapW;
      Stone winner = simulatePlayout(sim, simCapB_copy, simCapW_copy, simTurn, rng_local);
      for(MCTSNode* n : path){ n->vloss.fetch_sub(1); n->visits.fetch_add(1); if(n->playerJustMoved == winner) n->wins.fetch_add(1); }
    }
  };

  for(int i=0;i<numThreads;i++) workers.emplace_back(worker, i);
  for(auto &t: workers) t.join();

  MCTSNode* best=nullptr; size_t bestV=0;
  for(auto &cptr: rootPtr->children) {
    size_t v = cptr->visits.load();
    if(v > bestV){ best = cptr.get(); bestV = v; }
  }
  if(!best) return {-1,-1};
  return best->move;
}

// Background thinker: manager that repeatedly runs parallel MCTS chunks against a stored root.
struct BackgroundThinker{
  std::atomic<bool> running{false};
  std::thread mgr;
  std::mutex m;
  std::condition_variable cv;
  int threads = 4;
  int chunk = 200; // iterations per cycle
  double Cp = 1.414;
  Board boardSnapshot{9};
  int capB=0, capW=0;
  Stone toMove = BLACK;
  std::unique_ptr<MCTSNode> *rootPtrRef = nullptr;
  bool hasRoot=false;

  void setParams(int t, int c, double cp){ threads = t; chunk = c; Cp = cp; }

  void setRoot(const Board &b, int cb, int cw, Stone tm, std::unique_ptr<MCTSNode> *rptr){
    std::lock_guard<std::mutex> lk(m);
    boardSnapshot = b;
    capB = cb; capW = cw; toMove = tm;
    rootPtrRef = rptr;
    hasRoot = true;
    cv.notify_one();
  }

  void start(){ if(running.load()) return; running.store(true); mgr = std::thread([this]{ run(); }); }
  void stop(){ if(!running.load()) return; running.store(false); cv.notify_one(); if(mgr.joinable()) mgr.join(); }

  void run(){
    while(running.load()){
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, std::chrono::milliseconds(100), [this]{ return !running.load() || hasRoot; });
      if(!running.load()) break;
      if(!hasRoot || !rootPtrRef) { continue; }
      // capture snapshot locally
      Board b = boardSnapshot; int cb = capB, cw = capW; Stone tm = toMove; auto rptr = rootPtrRef;
      lk.unlock();
      // run a chunk of parallel iterations to improve the root
      mctsParallel(b, cb, cw, tm, size_t(chunk), threads, Cp, *rptr);
    }
  }
};


int main(){
  int N = 9;
  cout << "Go console (game-focus)" << endl;
  cout << "Board size (Enter for 9): ";
  std::string line;
  // If GO_AUTORUN env var is set, read lines into `autorunLines` so the program can be driven by a script.
  const char *autorun = std::getenv("GO_AUTORUN");
  std::vector<std::string> autorunLines;
  size_t autorunIndex = 0;
  if(autorun){ std::ifstream f(autorun); if(f.is_open()){ std::string l; while(std::getline(f,l)) autorunLines.push_back(l); } }
  if(!autorunLines.empty()){
    if(autorunIndex < autorunLines.size()) { line = autorunLines[autorunIndex++]; }
    else return 0;
  } else {
    if(!std::getline(cin, line)) return 0;
  }
  if(!line.empty()) try{ N = std::stoi(line); }catch(...){}
  Board board(N);
  Stone turn = BLACK;
  int capB=0, capW=0;

  // history for undo: store successive board states (initial state included)
  std::vector<Board> history;
  std::vector<int> histCapB, histCapW;
  std::vector<Stone> histTurns;
  auto pushHistory = [&](const Board &b){ history.push_back(b); histCapB.push_back(capB); histCapW.push_back(capW); histTurns.push_back(turn); if(history.size() > 300){ history.erase(history.begin()); histCapB.erase(histCapB.begin()); histCapW.erase(histCapW.begin()); histTurns.erase(histTurns.begin()); } };
  pushHistory(board);

  std::mt19937_64 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());

  std::unique_ptr<MCTSNode> mctsRoot;
  // Play-with-AI mode settings
  bool playWithAI = false;
  Stone aiPlays = WHITE; // which color AI will play when playWithAI==true
  double aiSeconds = 2.0;
  int aiThreads = 4;
  double aiCp = 1.414;
  BackgroundThinker bgThinker;

  #ifdef _WIN32
  // enable ANSI escape processing on Windows terminals to support VT sequences
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if(hOut != INVALID_HANDLE_VALUE){
    DWORD mode = 0;
    if(GetConsoleMode(hOut, &mode)){
      SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
  }
  #endif

  while(true){
    std::pair<int,int> lastMove = {-1,-1};
    auto mvlist = board.moves();
    if(!mvlist.empty()){
      auto lm = mvlist.back(); if(!lm.pass) lastMove = {lm.x, lm.y};
    }
    printBoard(board, lastMove);
    cout << "Captured: Black="<<capB<<" White="<<capW<<"\n";
    int legalCount = 0; for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(board.isLegal(x,y,turn)) ++legalCount;
    cout << (turn==BLACK?"Black":"White")<<" to move. "<<legalCount<<" legal moves. Commands: mcts [iters] | mctst [sec] | playai [B|W|both] [secs] [threads] [Cp] | stopai | ai | pass | undo | score | quit\n";
    cout << "Enter: ";
    // If play-with-AI is enabled and it's the AI's turn, make AI move automatically
    if(playWithAI && (turn==aiPlays || aiPlays==EMPTY)){
      // aiPlays==EMPTY means both sides AI
      // ensure background thinker follows parameters and root
      bgThinker.setParams(aiThreads, 200, aiCp);
      bgThinker.setRoot(board, capB, capW, turn, &mctsRoot);
      // prefer to run a blocking move decision (fast) but background think may have improved root
      auto mv = mctsParallelTimed(board, capB, capW, turn, aiSeconds, aiThreads, aiCp, mctsRoot);
      if(mv.first==-1){ board.pass(turn); pushHistory(board); if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, {-1,-1}); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); } bgThinker.setRoot(board, capB, capW, (turn==BLACK?WHITE:BLACK), &mctsRoot); turn = (turn==BLACK?WHITE:BLACK); continue; }
      if(board.place(mv.first,mv.second,turn)){ pushHistory(board); }
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, mv); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); }
      bgThinker.setRoot(board, capB, capW, (turn==BLACK?WHITE:BLACK), &mctsRoot);
      turn = (turn==BLACK?WHITE:BLACK);
      continue;
    }
    if(!std::getline(cin,line)) break;
    if(line.empty()) continue;
    if(line=="quit"||line=="q") break;
    if(line.rfind("playai",0)==0){
      std::istringstream iss(line); std::string cmd; iss>>cmd;
      std::string side; iss>>side;
      if(!side.empty()){
        if(side=="B"||side=="b") aiPlays = BLACK;
        else if(side=="W"||side=="w") aiPlays = WHITE;
        else if(side=="both") aiPlays = EMPTY;
      }
      double secs; int threads; double Cp;
      if(iss >> secs) aiSeconds = secs; if(iss >> threads) aiThreads = threads; if(iss >> Cp) aiCp = Cp;
      playWithAI = true;
      // start background thinker when enabling playai
      bgThinker.setParams(aiThreads, 200, aiCp);
      bgThinker.setRoot(board, capB, capW, turn, &mctsRoot);
      bgThinker.start();
      cout<<"play-with-AI enabled: aiPlays="<<(aiPlays==BLACK?"B":(aiPlays==WHITE?"W":"both"))<<" secs="<<aiSeconds<<" threads="<<aiThreads<<" Cp="<<aiCp<<"\n";
      continue;
    }
    if(line.rfind("stopai",0)==0 || line.rfind("playai off",0)==0){ playWithAI = false; bgThinker.stop(); cout<<"play-with-AI disabled\n"; continue; }
    if(line.rfind("mcts",0)==0){ size_t iters=500; double Cp=1.414; std::istringstream iss(line); std::string cmd; iss>>cmd; iss>>iters; iss>>Cp; auto mv = mctsMove(board, capB, capW, turn, iters, Cp, rng, mctsRoot);
      if(mv.first==-1){ board.pass(turn); turn = (turn==BLACK?WHITE:BLACK); pushHistory(board); }
      else { if(board.place(mv.first,mv.second,turn)) { /*captures tracked by board if needed*/ } turn = (turn==BLACK?WHITE:BLACK); pushHistory(board); }
      // attempt to reuse subtree: the chosen move should correspond to a child of current root
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, mv); if(newRoot) { mctsRoot = std::move(newRoot); } else { mctsRoot.reset(); } }
      // notify background thinker of the new root and turn
      bgThinker.setRoot(board, capB, capW, turn, &mctsRoot);
      continue; }
    if(line.rfind("mctst",0)==0){ double secs=2.0; double Cp=1.414; std::istringstream iss(line); std::string cmd; iss>>cmd; iss>>secs; iss>>Cp; if(secs<=0) secs = 2.0; auto mv = mctsMoveTimed(board, capB, capW, turn, secs, Cp, rng, mctsRoot);
      if(mv.first==-1){ board.pass(turn); turn = (turn==BLACK?WHITE:BLACK); pushHistory(board); }
      else { if(board.place(mv.first,mv.second,turn)){} turn = (turn==BLACK?WHITE:BLACK); pushHistory(board); }
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, mv); if(newRoot) { mctsRoot = std::move(newRoot); } else { mctsRoot.reset(); } }
      bgThinker.setRoot(board, capB, capW, turn, &mctsRoot);
      continue; }
    if(line.rfind("ai",0)==0){
      // ai mcts [secs] [threads] [Cp]  -> run MCTS; otherwise random move
      std::istringstream iss(line); std::string cmd, mode; iss>>cmd>>mode;
      if(mode=="mcts"){
        double secs = 2.0; int threads = 4; double Cp = 1.414;
        if(iss>>secs){} if(iss>>threads){} if(iss>>Cp){}
        auto mv = mctsParallelTimed(board, capB, capW, turn, secs, threads, Cp, mctsRoot);
        if(mv.first==-1){ board.pass(turn); pushHistory(board); if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, {-1,-1}); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); } bgThinker.setRoot(board, capB, capW, (turn==BLACK?WHITE:BLACK), &mctsRoot); turn = (turn==BLACK?WHITE:BLACK); continue; }
        if(board.place(mv.first,mv.second,turn)){ pushHistory(board); }
        if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, mv); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); }
        bgThinker.setRoot(board, capB, capW, (turn==BLACK?WHITE:BLACK), &mctsRoot);
        turn = (turn==BLACK?WHITE:BLACK);
        continue;
      }
      // fallback: simple random move
      std::vector<std::pair<int,int>> moves;
      for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(board.isLegal(x,y,turn)) moves.emplace_back(x,y);
      if(moves.empty()){ board.pass(turn); pushHistory(board); if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, {-1,-1}); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); } bgThinker.setRoot(board, capB, capW, (turn==BLACK?WHITE:BLACK), &mctsRoot); turn = (turn==BLACK?WHITE:BLACK); continue; }
      std::uniform_int_distribution<size_t> dist(0, moves.size()-1);
      auto p = moves[dist(rng)]; board.place(p.first,p.second,turn);
      // update history and try subtree reuse
      pushHistory(board);
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, p); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); }
      bgThinker.setRoot(board, capB, capW, (turn==BLACK?WHITE:BLACK), &mctsRoot);
      turn = (turn==BLACK?WHITE:BLACK);
      continue;
    }
    if(line=="undo"||line=="u"){ if(history.size()<=1){ cout<<"Nothing to undo"<<endl; continue; } history.pop_back(); histCapB.pop_back(); histCapW.pop_back(); histTurns.pop_back(); board = history.back(); capB = histCapB.back(); capW = histCapW.back(); turn = histTurns.back(); bgThinker.setRoot(board, capB, capW, turn, &mctsRoot); continue; }
    if(line=="pass"||line=="p"){ board.pass(turn); pushHistory(board); // update subtree for pass move
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, {-1,-1}); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); }
      bgThinker.setRoot(board, capB, capW, (turn==BLACK?WHITE:BLACK), &mctsRoot);
      turn = (turn==BLACK?WHITE:BLACK); continue; }
    if(line=="score"){ int bTerr=0,wTerr=0; /* compute territory simple */ cout<<"Scoring not implemented here."<<endl; std::getline(cin,line); continue; }

    int x=0,y=0;
    if(!parseCoord(line,x,y)){ cout<<"Invalid input"<<endl; continue; }
    if(!board.isLegal(x,y,turn)){ cout<<"Illegal move"<<endl; continue; }
    board.place(x,y,turn);
    // update subtree reuse for human move
    if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, {x,y}); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); }
    bgThinker.setRoot(board, capB, capW, (turn==BLACK?WHITE:BLACK), &mctsRoot);
    turn = (turn==BLACK?WHITE:BLACK);
  }
  cout<<"Bye"<<endl;
  // If autorun was used, write a simple final summary to a file for automated tests
  if(!autorunLines.empty()){
    std::string outpath = "D:/go/automation/game_result.txt";
    std::ofstream fout(outpath);
    if(fout.is_open()){
      int N = board.size();
      int blackTerr=0, whiteTerr=0;
      std::vector<char> seen(N*N,0);
      const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
      for(int y=0;y<N;y++) for(int x=0;x<N;x++){
        int id = y*N + x; if(seen[id]) continue;
        if(board.get(x,y) != EMPTY){ seen[id]=1; continue; }
        std::vector<int> stack; stack.push_back(id); seen[id]=1;
        bool adjB=false, adjW=false; int regionCount=0;
        while(!stack.empty()){ int cur=stack.back(); stack.pop_back(); regionCount++; int cx=cur%N, cy=cur/N;
          for(int i=0;i<4;i++){ int nx=cx+dx[i], ny=cy+dy[i]; if(nx<0||ny<0||nx>=N||ny>=N) continue; int nid = ny*N+nx; Stone s = board.get(nx,ny); if(s==EMPTY && !seen[nid]){ seen[nid]=1; stack.push_back(nid);} else if(s==BLACK) adjB=true; else if(s==WHITE) adjW=true; }
        }
        if(adjB && !adjW) blackTerr += regionCount; else if(adjW && !adjB) whiteTerr += regionCount;
      }
      int blackStones=0, whiteStones=0;
      for(int y=0;y<N;++y) for(int x=0;x<N;++x){ Stone s = board.get(x,y); if(s==BLACK) ++blackStones; else if(s==WHITE) ++whiteStones; }
      int blackTotal = blackStones + capB + blackTerr; int whiteTotal = whiteStones + capW + whiteTerr;
      fout << "Final score summary\n";
      fout << "Black stones="<<blackStones<<" cap="<<capB<<" terr="<<blackTerr<<" total="<<blackTotal<<"\n";
      fout << "White stones="<<whiteStones<<" cap="<<capW<<" terr="<<whiteTerr<<" total="<<whiteTotal<<"\n";
      fout << "Winner: " << (blackTotal>whiteTotal?"Black":(whiteTotal>blackTotal?"White":"Tie")) << "\n";
      fout.close();
    }
  }
  return 0;
}
