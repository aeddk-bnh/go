#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <thread>
#include <memory>
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
  size_t wins=0, visits=0;
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
        if(c->visits==0){ best = c; break; }
        double wr = double(c->wins)/double(c->visits);
        double uct = wr + Cp * std::sqrt(std::log(double(node->visits))/double(c->visits));
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
    }

    // simulation
    int simCapB_copy = simCapB, simCapW_copy = simCapW;
    Stone winner = simulatePlayout(sim, simCapB_copy, simCapW_copy, simTurn, rng);

    // backprop
    MCTSNode* back = node;
    while(back!=nullptr){ back->visits += 1; if(back->playerJustMoved == winner) back->wins += 1; back = back->parent; }
  }

  // choose most visited child
  MCTSNode* best=nullptr; size_t bestV=0;
  for(auto &cptr: rootPtr->children) if(cptr->visits > bestV){ best = cptr.get(); bestV = cptr->visits; }
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

  size_t it = 0;
  while(clock::now() < deadline){
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
        if(c->visits==0){ best = c; break; }
        double wr = double(c->wins)/double(c->visits);
        double uct = wr + Cp * std::sqrt(std::log(double(node->visits))/double(c->visits));
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
      node->untried.erase(node->untried.begin()+idx);
      if(mv.first != -1) sim.place(mv.first,mv.second,simTurn);
      else sim.pass(simTurn);
      Stone pjm = simTurn;
      simTurn = (simTurn==BLACK?WHITE:BLACK);
      std::vector<std::pair<int,int>> childMoves;
      for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(sim.isLegal(x,y,simTurn)) childMoves.emplace_back(x,y);
      childMoves.emplace_back(-1,-1);
      node->children.emplace_back(std::make_unique<MCTSNode>(node, childMoves, mv, pjm));
      node = node->children.back().get();
    }

    // simulation
    int simCapB_copy = simCapB, simCapW_copy = simCapW;
    Stone winner = simulatePlayout(sim, simCapB_copy, simCapW_copy, simTurn, rng);

    // backprop
    MCTSNode* back = node;
    while(back!=nullptr){ back->visits += 1; if(back->playerJustMoved == winner) back->wins += 1; back = back->parent; }

    ++it;
    // occasional deadline check already handled by while condition
  }

  // choose most visited child
  MCTSNode* best=nullptr; size_t bestV=0;
  for(auto &cptr: rootPtr->children) if(cptr->visits > bestV){ best = cptr.get(); bestV = cptr->visits; }
  if(!best) return {-1,-1};
  return best->move;
}

int main(){
  int N = 9;
  cout << "Go console (game-focus)" << endl;
  cout << "Board size (Enter for 9): ";
  std::string line;
  if(!std::getline(cin, line)) return 0;
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
    cout << (turn==BLACK?"Black":"White")<<" to move. "<<legalCount<<" legal moves. Commands: mcts [iters] | mctst [sec] | ai | pass | undo | score | quit\n";
    cout << "Enter: ";
    if(!std::getline(cin,line)) break;
    if(line.empty()) continue;
    if(line=="quit"||line=="q") break;
    if(line.rfind("mcts",0)==0){ size_t iters=500; double Cp=1.414; std::istringstream iss(line); std::string cmd; iss>>cmd; iss>>iters; iss>>Cp; auto mv = mctsMove(board, capB, capW, turn, iters, Cp, rng, mctsRoot); if(mv.first==-1){ board.pass(turn); turn = (turn==BLACK?WHITE:BLACK); pushHistory(board); } else { if(board.place(mv.first,mv.second,turn)) { /*captures tracked by board if needed*/ } turn = (turn==BLACK?WHITE:BLACK); pushHistory(board); }
      // attempt to reuse subtree: the chosen move should correspond to a child of current root
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, mv); if(newRoot) { mctsRoot = std::move(newRoot); } else { mctsRoot.reset(); } }
      continue; }
    if(line.rfind("mctst",0)==0){ double secs=2.0; double Cp=1.414; std::istringstream iss(line); std::string cmd; iss>>cmd; iss>>secs; iss>>Cp; if(secs<=0) secs = 2.0; auto mv = mctsMoveTimed(board, capB, capW, turn, secs, Cp, rng, mctsRoot); if(mv.first==-1){ board.pass(turn); turn = (turn==BLACK?WHITE:BLACK); pushHistory(board); } else { if(board.place(mv.first,mv.second,turn)){} turn = (turn==BLACK?WHITE:BLACK); pushHistory(board); }
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, mv); if(newRoot) { mctsRoot = std::move(newRoot); } else { mctsRoot.reset(); } }
      continue; }
    if(line=="ai"){ // simple random move
      std::vector<std::pair<int,int>> moves;
      for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(board.isLegal(x,y,turn)) moves.emplace_back(x,y);
      if(moves.empty()){ board.pass(turn); pushHistory(board); if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, {-1,-1}); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); } turn = (turn==BLACK?WHITE:BLACK); continue; }
      std::uniform_int_distribution<size_t> dist(0, moves.size()-1);
      auto p = moves[dist(rng)]; board.place(p.first,p.second,turn);
      // update history and try subtree reuse
      pushHistory(board);
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, p); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); }
      turn = (turn==BLACK?WHITE:BLACK);
      continue;
    }
    if(line=="undo"||line=="u"){ if(history.size()<=1){ cout<<"Nothing to undo"<<endl; continue; } history.pop_back(); histCapB.pop_back(); histCapW.pop_back(); histTurns.pop_back(); board = history.back(); capB = histCapB.back(); capW = histCapW.back(); turn = histTurns.back(); continue; }
    if(line=="pass"||line=="p"){ board.pass(turn); pushHistory(board); // update subtree for pass move
      if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, {-1,-1}); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); }
      turn = (turn==BLACK?WHITE:BLACK); continue; }
    if(line=="score"){ int bTerr=0,wTerr=0; /* compute territory simple */ cout<<"Scoring not implemented here."<<endl; std::getline(cin,line); continue; }

    int x=0,y=0;
    if(!parseCoord(line,x,y)){ cout<<"Invalid input"<<endl; continue; }
    if(!board.isLegal(x,y,turn)){ cout<<"Illegal move"<<endl; continue; }
    board.place(x,y,turn);
    // update subtree reuse for human move
    if(mctsRoot){ auto newRoot = detachChildByMove(mctsRoot, {x,y}); if(newRoot) mctsRoot = std::move(newRoot); else mctsRoot.reset(); }
    turn = (turn==BLACK?WHITE:BLACK);
  }
  cout<<"Bye"<<endl;
  return 0;
}
