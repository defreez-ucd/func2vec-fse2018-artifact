#ifndef HANDLERS_PASS
#define HANDLERS_PASS

#include "Location.hpp"
#include "FlowGraph.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include <vector>

// This is a FunctionPass because I could not figure out
// how to get MDA to run from a ModulePass
class HandlersPass : public llvm::FunctionPass {
public:
  static char ID;
  HandlersPass() : FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
   
  // List of branch locations that are potentially error handlers
  // Uses by Traces
  std::vector<Location> branches;

  // Maps a branch location to the function returning the value being tested
  // Used by Traces
  std::map<Location, std::string> returning_functions;

  // Used by Llvm->Context
  // Sloppy
  std::map<std::string, std::string> returning_functions_by_id;

  // Sloppy
  std::map<std::string, std::string> handler_to_branch;

  // Data structures backing getEHPair()
  // Maps stack -> stack, stack, stack (NOT_EH, EH, EH_END)
  std::map<std::string, std::tuple<std::string, std::string, std::string>> eh_vertices;

private:
  void addBranchIfCall(llvm::BranchInst *branch, llvm::Value *maybe_call);

  // Populates eh_pairs map
  void fillEHVertices(llvm::Function &F);

  std::unordered_set<llvm::BranchInst*> eh_branches;
};

#endif
