#include "sgf.h"
#include <cctype>
#include <sstream>

static int letterToCoord(char c){ return c - 'a'; }
static char coordToLetter(int v){ return char('a' + v); }

namespace SGF {

bool parse(const std::string& sgf, Board& out, double& komi_out){
  komi_out = 0.0;
  int sz = out.size();
  // find SZ
  auto szpos = sgf.find("SZ[");
  if(szpos!=std::string::npos){
    auto p = sgf.find(']', szpos+3);
    if(p!=std::string::npos){
      std::string val = sgf.substr(szpos+3, p-(szpos+3));
      int v = std::stoi(val);
      // If size conflicts, ignore (we keep board size as is)
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
    }
  }

  // parse nodes and moves
  size_t i=0; while(i<sgf.size()){
    if(sgf[i]==';'){
      i++;
      // read properties until next ';' or ')'
      while(i<sgf.size() && sgf[i]!=';' && sgf[i]!=')'){
        // skip whitespace
        if(std::isspace((unsigned char)sgf[i])){ i++; continue; }
        // property name
        size_t j=i; while(j<sgf.size() && std::isupper((unsigned char)sgf[j])) j++;
        std::string prop = sgf.substr(i, j-i);
        i = j;
        // expect bracket
        if(i<sgf.size() && sgf[i]=='['){
          i++;
          size_t k=i; while(k<sgf.size() && sgf[k]!=']') k++;
          std::string val = sgf.substr(i, k-i);
          i = k+1;
          if(prop=="B" || prop=="W"){
            Stone s = (prop=="B"?BLACK:WHITE);
            if(val.size()==0){ out.pass(s); }
            else if(val.size()>=2){ int x=letterToCoord(val[0]); int y=letterToCoord(val[1]); out.place(x,y,s); }
          }
          // ignore other properties for now
        }
      }
    } else i++;
  }
  return true;
}

std::string write(const Board& b, double komi){
  std::ostringstream ss;
  ss << "(\n";
  ss << ";GM[1]FF[4]SZ["<<b.size()<<"]KM["<<komi<<"]";
  for(auto &m : b.moves()){
    if(m.pass){ ss << ";" << (m.s==BLACK?"B":"W") << "[]"; }
    else { ss << ";" << (m.s==BLACK?"B":"W") << "[" << coordToLetter(m.x) << coordToLetter(m.y) << "]"; }
  }
  ss << ")\n";
  return ss.str();
}

} // namespace SGF