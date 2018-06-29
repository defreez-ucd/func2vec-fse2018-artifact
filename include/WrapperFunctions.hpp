#ifndef WRAPPER_FUNCTIONS_HPP
#define WRAPPER_FUNCTIONS_HPP

#include <llvm/Pass.h>

using namespace std;
using namespace llvm;

class WrapperFunctions : public ModulePass {
  
public:
  WrapperFunctions() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M);

  bool runOnFunction(Function &F);
  
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  static char ID; // Pass identification, replacement for typeid

};

#endif // WRAPPER_FUNCTIONS_HPP
