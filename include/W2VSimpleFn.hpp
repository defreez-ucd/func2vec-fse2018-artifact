#ifndef W2V_SIMPLE_FN_PASS
#define W2V_SIMPLE_FN_PASS

#include "llvm/Pass.h"

class W2VSimpleFnPass : public llvm::ModulePass {
public:
  static char ID;
  W2VSimpleFnPass() : ModulePass(ID) {}
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnModule(llvm::Module &M) override;
};

#endif
