#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <memory>
#include <limits>
#include <algorithm>
#include <iomanip>

#include "board.h"

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

void printBoard(const Board &b){
  clearScreen();
  int N = b.size();
  cout << "   ";
  for(int x=0;x<N;++x){ cout << ' ' << char('A'+x) << ' '; }
  cout << '\n';
  for(int y=0;y<N;++y){
    cout << std::setw(2) << (y+1) << ' ';
    for(int x=0;x<N;++x){
      Stone s = b.get(x,y);
      const char *c = (s==BLACK)?"\u25CF":(s==WHITE)?"\u25CB":".";
      cout << ' ' << c << ' ';
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
      int beforeEnemy = sim.count(simTurn==BLACK?WHITE:BLACK);
      bool ok = sim.place(pick.first,pick.second,simTurn);
      if(!ok){ consecutivePasses++; simTurn = (simTurn==BLACK?WHITE:BLACK); }
      else{
        int afterEnemy = sim.count(simTurn==BLACK?WHITE:BLACK);
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
  int blackTotal = sim.count(BLACK) + simCapB + blackTerr;
  int whiteTotal = sim.count(WHITE) + simCapW + whiteTerr;
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

static std::pair<int,int> mctsMove(const Board &rootBoard, int rootCapB, int rootCapW, Stone toMove, size_t iterations, double Cp, std::mt19937_64 &rng){
  Stone rootPJM = (toMove==BLACK?WHITE:BLACK);
  std::vector<std::pair<int,int>> rootMoves;
  int N = rootBoard.size();
  for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(rootBoard.isLegal(x,y,toMove)) rootMoves.emplace_back(x,y);
  rootMoves.emplace_back(-1,-1);
  MCTSNode root(nullptr, rootMoves, {-2,-2}, rootPJM);

  for(size_t it=0; it<iterations; ++it){
    MCTSNode* node = &root;
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
  for(auto &cptr: root.children) if(cptr->visits > bestV){ best = cptr.get(); bestV = cptr->visits; }
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

  std::mt19937_64 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());

  while(true){
    printBoard(board);
    cout << "Captured: Black="<<capB<<" White="<<capW<<"\n";
    cout << (turn==BLACK?"Black":"White")<<" to move. Commands: mcts [iters] | ai | pass | undo(not supported) | score | quit\n";
    cout << "Enter: ";
    if(!std::getline(cin,line)) break;
    if(line.empty()) continue;
    if(line=="quit"||line=="q") break;
    if(line.rfind("mcts",0)==0){ size_t iters=500; double Cp=1.414; std::istringstream iss(line); std::string cmd; iss>>cmd; iss>>iters; iss>>Cp; auto mv = mctsMove(board, capB, capW, turn, iters, Cp, rng); if(mv.first==-1){ board.pass(turn); turn = (turn==BLACK?WHITE:BLACK); } else { if(board.place(mv.first,mv.second,turn)) { int before = 0; /*captures tracked by board if needed*/ } turn = (turn==BLACK?WHITE:BLACK); } continue; }
    if(line=="ai"){ // simple random move
      std::vector<std::pair<int,int>> moves;
      for(int y=0;y<N;y++) for(int x=0;x<N;x++) if(board.isLegal(x,y,turn)) moves.emplace_back(x,y);
      if(moves.empty()){ board.pass(turn); turn = (turn==BLACK?WHITE:BLACK); continue; }
      std::uniform_int_distribution<size_t> dist(0, moves.size()-1);
      auto p = moves[dist(rng)]; board.place(p.first,p.second,turn); turn = (turn==BLACK?WHITE:BLACK); continue;
    }
    if(line=="pass"||line=="p"){ board.pass(turn); turn = (turn==BLACK?WHITE:BLACK); continue; }
    if(line=="score"){ int bTerr=0,wTerr=0; /* compute territory simple */ cout<<"Scoring not implemented here."<<endl; std::getline(cin,line); continue; }

    int x=0,y=0;
    if(!parseCoord(line,x,y)){ cout<<"Invalid input"<<endl; continue; }
    if(!board.isLegal(x,y,turn)){ cout<<"Illegal move"<<endl; continue; }
    board.place(x,y,turn);
    turn = (turn==BLACK?WHITE:BLACK);
  }
  cout<<"Bye"<<endl;
  return 0;
}
