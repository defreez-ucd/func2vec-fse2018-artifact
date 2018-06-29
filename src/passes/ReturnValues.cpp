#include "ReturnValuesPass.hpp"
#include "ControlFlow.hpp"
#include <llvm/IR/Module.h>
#include "llvm/IR/InstIterator.h"
#include <llvm/IR/InstrTypes.h>
#include <unordered_set>
#include <iostream>
#include <string>

using namespace llvm;
using namespace std;

char ReturnValuesPass::ID = 0;
static RegisterPass<ReturnValuesPass> X("return-values", "Return Values", false, false);

bool ReturnValuesPass::runOnModule(Module &M) {
  cerr << "RUN RETURN VALUES PASS\n";
  
  for (auto f = M.begin(), e = M.end(); f != e; ++f) {
    Function *fn = &*f;
    runOnFunction(*fn);
  }  

  return false;
}

void ReturnValuesPass::runOnFunction(Function &F) {
  unordered_set<Value*> need_lookup;
  unordered_set<Value*> inst_write_to_returns;
  
  for (inst_iterator i = inst_begin(F), ie = inst_end(F); i != ie; ++i) {
    Instruction *inst = &*i;
    if (ReturnInst *ret = dyn_cast<ReturnInst>(inst)) {
      Value *to_lookup = handleReturn(*ret);
      if (to_lookup == ret) {
        inst_write_to_returns.insert(ret);
      } else if (to_lookup) {
        need_lookup.insert(to_lookup);
      } else {
        cerr << "WARNING: Unable to determine where return value is set." << endl;
      }
    }
  }

  for (inst_iterator i = inst_begin(F), ie = inst_end(F); i != ie; ++i) {
    Instruction *inst = &*i;    
    if (StoreInst *store = dyn_cast<StoreInst>(inst)) {
      Value *sender = store->getOperand(1);
      if (need_lookup.find(sender) != need_lookup.end()) {
        inst_write_to_returns.insert(store);
      }
    }
  }
  
  ControlFlowPass &cfp = getAnalysis<ControlFlowPass>();
  
  // Mark nodes in control flow graph
  BGL_FORALL_VERTICES(v, cfp.FG.G, _FlowGraph) {
    if (inst_write_to_returns.find(cfp.FG.G[v].I) != inst_write_to_returns.end()) {
      cerr << cfp.FG.G[v].stack << " IS RETURN WRITE\n";
      cerr << v << endl;
      write_to_return.insert(cfp.FG.G[v].stack);
    }
  }
}

// Returns null if do not need to lookup
Value *ReturnValuesPass::handleReturn(ReturnInst &ret) {
  if (ret.getNumOperands() == 0) {
    return &ret;
  }
  
  Value *return_value = ret.getOperand(0); 
  if (isa<Constant>(return_value)) {
    return &ret;
  }

  if (LoadInst *load = dyn_cast<LoadInst>(return_value)) {
    return load->getOperand(0);  
  }

  return nullptr;
}

void ReturnValuesPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<ControlFlowPass>();
}
