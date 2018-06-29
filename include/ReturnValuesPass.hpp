#ifndef RETURNVALUES_H
#define RETURNVALUES_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "FlowGraph.hpp"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugInfo.h"
#include <llvm/IR/Instructions.h>

class ReturnValuesPass : public llvm::ModulePass {
public:
  static char ID;
  ReturnValuesPass() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  std::unordered_set<std::string> write_to_return;
  
private:
  void runOnFunction(llvm::Function &F);
  llvm::Value* handleReturn(llvm::ReturnInst &ret);  
};

#endif
