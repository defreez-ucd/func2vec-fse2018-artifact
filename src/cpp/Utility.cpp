#include "Utility.hpp"
#include "llvm/Support/raw_ostream.h"
#include <fstream>

using namespace llvm;
using namespace std;

// Separate namespaces for error-prop and program2vec

namespace ep {
Location getSource(Instruction *I) {
  if (I == nullptr) {
    errs() << "getSource requested for nullptr\n";
    abort();
  }

  Location location;
  DILocation *loc = I->getDebugLoc();
  
  if (loc) {
    std::string file = loc->getFilename();
    unsigned line = loc->getLine();
    std::string dir = loc->getDirectory();

    location = Location(file, line);
  }
 
  return location;
}
  
};

namespace p2v {
  // Read the list of interesting functions from a file
  // One function name per line
  unordered_set<string> read_interesting_functions(string path) {
    unordered_set<string> ret;
    ifstream f_functions(path);
    string line;
    
    while (getline(f_functions, line)) {
      ret.insert(line);
    }

    return ret;
  }

  // For some functions LLVM will
  string getName(llvm::Function &F) {
    string name = F.getName();
    auto idx = name.find('.');
    if (idx != string::npos) {
      name = name.substr(0, idx);
    }
    return name;
  }
};
