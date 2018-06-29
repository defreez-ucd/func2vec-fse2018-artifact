#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <iostream>
#include <string>
#include <set>

using namespace llvm;
using namespace std;

// This is a function pass not a module pass.
// Therefore we don't need to explicitly strip out intrinsics and declarations.
struct IfStatements : public FunctionPass {
  static char ID;
  IfStatements() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    string fname = F.getName().str();
    if (fname.find("_create") == string::npos) {
      return false;
    }

    for (inst_iterator i = inst_begin(F), ie = inst_end(F); i != ie; ++i) {
      Instruction *inst = &*i;
      if (BranchInst * br = dyn_cast<BranchInst>(inst)) {
        if (br->isUnconditional()) {
          continue;
        }
        if (DILocation * loc = inst->getDebugLoc()) {
          unsigned line = loc->getLine();
          string file = loc->getFilename();
          if (file.find("pci") == string::npos) {
            continue;
          }
          cout << file << " " << line << endl;
        }
      }
    }

    return false;
  }

};

char IfStatements::ID = 0;
static RegisterPass<IfStatements> X("ifstatements", "if statements", false, false);
