#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#include <iostream>
#include <string>

using namespace llvm;
using namespace std;

// This is a function pass not a module pass.
// Therefore we don't need to explicitly strip out intrinsics and declarations.
struct DefinedFunctions : public FunctionPass {
  static char ID;
  DefinedFunctions() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    string fname = F.getName().str();
    auto idx = fname.find('.');
    if (idx != string::npos) {
      fname = fname.substr(0, idx);
    }

    // Getting metadata for the actual function information is a pain
    // (need to loop over all of the DISubprograms). We only need the
    // directory and filename here, so I just get the debug location of
    // the first instruction in the function that contains metadata.

    DILocation *loc = nullptr;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (loc = I->getDebugLoc()) {
        string file = loc->getFilename();
        cout << file << " " << fname << endl;
        break;
      }
    }
    assert(loc);
    return false;
  }

};

char DefinedFunctions::ID = 0;
static RegisterPass<DefinedFunctions> X("definedfunctions",
                                        "List functions defined in a bitcode file",
                                        false, false);
