#include "sgf.h"
#include <cctype>
#include <sstream>
#include <fstream>
#include <algorithm>

static int letterToCoord(char c){ return c - 'a'; }
static char coordToLetter(int v){ return char('a' + v); }

static std::string escapeText(const std::string &in){
  std::string out;
  for(char c:in){
    if(c=='\\' || c==']' || c==';') { out.push_back('\\'); out.push_back(c); }
    else if(c=='\n') { out.push_back('\\'); out.push_back('n'); }
    else if(c=='\r') { out.push_back('\\'); out.push_back('r'); }
    else out.push_back(c);
  }
  return out;
}

static std::string unescapeText(const std::string &in){
  std::string out;
  for(size_t i=0;i<in.size();++i){
    char c = in[i];
    if(c=='\\' && i+1<in.size()){
      char n = in[i+1];
      i++;
      if(n=='n') out.push_back('\n');
      else if(n=='r') out.push_back('\r');
      else out.push_back(n);
    } else out.push_back(c);
  }
  return out;
}

namespace SGF {

bool parse(const std::string& sgf, Board& out, double& komi_out, Game* game){
  komi_out = 0.0;
  if(game){ game->moves.clear(); game->PB.clear(); game->PW.clear(); game->RE.clear(); game->KM = 0.0; }
  (void)out; // size not tracked here
  // find SZ
  auto szpos = sgf.find("SZ[");
  if(szpos!=std::string::npos){
    auto p = sgf.find(']', szpos+3);
    if(p!=std::string::npos){
      std::string val = sgf.substr(szpos+3, p-(szpos+3));
      int v = std::stoi(val);
      (void)v;
    }
  }
  // find KM
  auto kmpos = sgf.find("KM[");
  if(kmpos!=std::string::npos){
    auto p = sgf.find(']', kmpos+3);
    if(p!=std::string::npos){
      std::string val = sgf.substr(kmpos+3, p-(kmpos+3));
      komi_out = std::stod(val);
      if(game) game->KM = komi_out;
    }
  }

  // parse nodes and moves (main line only)
  size_t i=0; while(i<sgf.size()){
    if(sgf[i]==';'){
      i++;
      // parse all properties in this node
      [[maybe_unused]] std::string nodePB, nodePW, nodeRE, nodeC;
      char moveColor=0; std::string moveVal;
      while(i<sgf.size() && sgf[i]!=';' && sgf[i]!=')' && sgf[i] != '('){
        if(std::isspace((unsigned char)sgf[i])){ i++; continue; }
        // property name
        size_t j=i; while(j<sgf.size() && std::isupper((unsigned char)sgf[j])) j++;
        std::string prop = sgf.substr(i, j-i);
        i = j;
        if(i<sgf.size() && sgf[i]=='['){
          i++; size_t k=i; while(k<sgf.size() && sgf[k]!=']') k++;
          std::string raw = sgf.substr(i, k-i);
          std::string val = unescapeText(raw);
          i = (k<sgf.size()?k+1:k);
          if(prop=="B" || prop=="W"){
            moveColor = prop[0]; moveVal = val;
          } else if(prop=="KM"){
            komi_out = std::stod(val);
            if(game) game->KM = komi_out;
          } else if(prop=="PB"){
            if(game) game->PB = val;
          } else if(prop=="PW"){
            if(game) game->PW = val;
          } else if(prop=="RE"){
            if(game) game->RE = val;
          } else if(prop=="C"){
            nodeC = val;
          }
        }
      }
      // apply move if any
      if(moveColor!=0){
        Stone s = (moveColor=='B')?BLACK:WHITE;
        if(moveVal.size()==0){ out.pass(s); out.setLastMoveComment(nodeC); if(game) game->moves.push_back({-1,-1,s,true,nodeC}); }
        else if(moveVal.size()>=2){ int x=letterToCoord(moveVal[0]); int y=letterToCoord(moveVal[1]); out.place(x,y,s); out.setLastMoveComment(nodeC); if(game) game->moves.push_back({x,y,s,false,nodeC}); }
      }
    } else i++;
  }
  return true;
}

std::string write(const Board& b, double komi){
  Game g;
  g.KM = komi;
  g.moves = b.moves();
  return write(g);
}

std::string write(const Game& g){
  std::ostringstream ss;
  ss << "(\n";
  ss << ";GM[1]FF[4]SZ[19]KM["<<g.KM<<"]"; // SZ is not tracked here
  if(!g.PB.empty()) ss << "PB["<<escapeText(g.PB)<<"]";
  if(!g.PW.empty()) ss << "PW["<<escapeText(g.PW)<<"]";
  if(!g.RE.empty()) ss << "RE["<<escapeText(g.RE)<<"]";
  for(auto &m : g.moves){
    ss << ";" << (m.s==BLACK?"B":"W");
    if(m.pass) ss << "[]";
    else ss << "[" << coordToLetter(m.x) << coordToLetter(m.y) << "]";
    if(!m.comment.empty()) ss << "C["<< escapeText(m.comment) << "]";
  }
  ss << ")\n";
  return ss.str();
}

bool readFile(const std::string& path, Board& out, double& komi_out, Game* game){
  std::ifstream in(path);
  if(!in) return false;
  std::ostringstream ss; ss << in.rdbuf();
  return parse(ss.str(), out, komi_out, game);
}

bool writeFile(const std::string& path, const Game& g){
  std::ofstream out(path);
  if(!out) return false;
  out << write(g);
  return true;
}

} // namespace SGF