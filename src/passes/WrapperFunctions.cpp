#include "WrapperFunctions.hpp"
#include "Utility.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace std;
using namespace llvm;

static const RegisterPass<WrapperFunctions> registration("find-wrappers", "Printing function names");

bool WrapperFunctions::runOnModule(Module &M) {
  for(Module::iterator f = M.begin(), fe = M.end(); f != fe; f++) {
    if (!f->isDeclaration()) {
      runOnFunction(*f);
    }
  }
  return false;
}

bool WrapperFunctions::runOnFunction(Function &F) {
  int blocks = 0, calls = 0;
  Function* function = NULL;
  for (Function::iterator b = F.begin(), be = F.end(); b != be; b++) {
    blocks++;
    for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; i++) {
      Instruction *inst = (Instruction*)i;
      if (CallInst *callInst = dyn_cast<CallInst>(inst)) {
        if (Function* callee = callInst->getCalledFunction()) {
          if (!callee->isIntrinsic()) {
            calls++;
            function = callee;
          }
        }
      }
    }
  }

  if (blocks == 1 && calls == 1) {
    errs() << p2v::getName(F) << " " << p2v::getName(*function) << "\n";
  }
  return false;
}  


void WrapperFunctions::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}


char WrapperFunctions::ID = 0;
