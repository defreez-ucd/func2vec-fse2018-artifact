// The purpose of this pass is to identify which branch of a conditional, if any, is an error check.

#ifndef BRANCHSAFETYPASS_H
#define BRANCHSAFETYPASS_H

#include "Names.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "Utility.hpp"
#include <fstream>
#include <set>

class BranchSafetyPass : public llvm::ModulePass  {
public:

  static char ID;
  BranchSafetyPass() : llvm::ModulePass(ID) {}
  BranchSafetyPass(string test_out_path) : llvm::ModulePass(ID), test_out_path(test_out_path) {
    testing = (! test_out_path.empty());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  bool runOnModule(llvm::Module &M) override;
  void visitBranchInst(llvm::BranchInst &I);

  // Gets the VarNames that do not hold errors in this basic block
  std::set<VarName> getSafeNames(llvm::BasicBlock &BB);

  // Returns pair (safe, unsafe) blocks, where unsafe is potentially error-handling
  std::pair<llvm::BasicBlock*, llvm::BasicBlock*> getBranchBlocks(llvm::BranchInst *branch);

  // Returns whether or not var is tested by icmp instruction
  bool tested_at(VarName var, llvm::ICmpInst *icmp);

  // Returns the error codes that can pass through this predicate
  std::set<VarName> codesThroughPred(llvm::ICmpInst *icmp);
  
private:
  NamesPass *names;

  // Values that we know are not ECs in a block
  map<llvm::BasicBlock*, set<llvm::Value*>> safe_values;

  map<llvm::Instruction*, std::pair<ep::TriVal, ep::TriVal>> branch_pairs;

  // Special case for handling IS_ERR tests
  llvm::Value* strip_is_err(llvm::Value* operand);

  // All of the icmp instructions that might test error codes
  // And the variable names they test
  std::map<llvm::ICmpInst*, std::set<VarName>> icmps;

  // Save explicit tests against single error codes
  std::map<llvm::ICmpInst*, VarName> explicit_equality;
  std::map<llvm::ICmpInst*, VarName> explicit_inequality;

  string test_out_path;
  ofstream test_out_file;
  bool testing = false;
};

class Branch {
public:

  void flip() {
    switch (direction) {
      case ep::TriVal::T:
        direction = ep::TriVal::F;
        break;
      case ep::TriVal::F:
        direction = ep::TriVal::T;
        break;
      default:
        break;
    }
  }

  string str() {
    switch (direction) {
      case ep::TriVal::T:
        return "T";
        break;
      case ep::TriVal::F:
        return "F";
        break;
      default:
        return "N";
        break;
    }
  }

  ep::TriVal direction = ep::TriVal::N;
};

#endif
