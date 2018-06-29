#ifndef FUNC2VEC_INSTRUCTIONLABELS_H
#define FUNC2VEC_INSTRUCTIONLABELS_H

#include "llvm/Pass.h"
#include "Names.hpp"
#include "ControlFlow.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"

struct LabelVisitor : llvm::InstVisitor<LabelVisitor> {
  LabelVisitor(NamesPass *names, FlowGraph &FG) : names(names), FG(FG) {}

  void visitInstruction(llvm::Instruction &I);

  void visitStoreInst(llvm::StoreInst &I);

  void visitReturnInst(llvm::ReturnInst &I);

  void visitBranchInst(llvm::BranchInst &I);

  void visitGetElementPtrInst(llvm::GetElementPtrInst &I);

  // function that that takes string -> id
  // generate id if string does not exist
  int getOrCreateId(const std::string &label);

  NamesPass *names;
  FlowGraph &FG;
  std::unordered_map<std::string, int> label_to_id;
};

class InstructionLabelsPass : public llvm::ModulePass {
public:
  static char ID;

  InstructionLabelsPass() : llvm::ModulePass(ID) {}

  virtual bool runOnModule(llvm::Module &M) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  std::unordered_map<std::string, int> label_to_id;
};


#endif //FUNC2VEC_INSTRUCTIONLABELS_H
