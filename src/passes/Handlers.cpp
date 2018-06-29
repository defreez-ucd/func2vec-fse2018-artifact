#include "HandlersPass.hpp"
#include "BranchSafety.hpp"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/PostDominators.h"

using namespace llvm;
using namespace std;

void HandlersPass::addBranchIfCall(BranchInst *branch, Value *maybe_call) {
  assert(branch);
  assert(maybe_call);

  NamesPass *names = &getAnalysis<NamesPass>();
  
  if (CallInst *call = dyn_cast<CallInst>(maybe_call)) {
    Location loc = ep::getSource(branch);
    branches.push_back(loc);

    Function *callee = call->getCalledFunction();
    if (callee) {
      returning_functions[loc] = callee->getName();
      string branch_id = names->getStackName(*branch);
      string name = callee->getName();
      if (name.find(".") != string::npos) {
        name = name.substr(0, name.find("."));
      }
      returning_functions_by_id[branch_id] = name;
    }
  }
}

void HandlersPass::fillEHVertices(Function &F) {
  BranchSafetyPass *safety = &getAnalysis<BranchSafetyPass>();
  PostDominatorTree *postdom = new PostDominatorTree();
  NamesPass *names = &getAnalysis<NamesPass>();

  // BranchSafety does not know about the FlowGraph, so we store the basic blocks
  // that are EH / NOT EH
  // Then we go through and fill the vertices map

  // Temporarily map by instruction until we can find which vertices are
  map<Instruction*, tuple<FlowVertex*, FlowVertex*, FlowVertex*>> eh_instructions;
  
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;
    if (BranchInst *branch = dyn_cast<BranchInst>(inst)) {
      pair<BasicBlock*, BasicBlock*> branch_blocks = safety->getBranchBlocks(branch);
      BasicBlock *not_handler_block = branch_blocks.first;
      BasicBlock *handler_block = branch_blocks.second;
      if (!handler_block) continue;

      // Handler is the empty else branch - do nothing
      postdom->runOnFunction(*handler_block->getParent());
      if (postdom->dominates(handler_block, not_handler_block)) continue;

      // Find where control flow merges again
      BasicBlock *join_block = postdom->findNearestCommonDominator(handler_block, not_handler_block);
      // No common post-dominator. Something funky going on.
      // We have to skip this error-handling block
      if (!join_block) return;

      string branch_id, not_handler_id, handler_on_id, handler_off_id;
      branch_id = names->getStackName(*branch);
      tie(not_handler_id, std::ignore)  = names->getBBNames(*not_handler_block);
      tie(handler_on_id, std::ignore)   = names->getBBNames(*handler_block);
      tie(handler_off_id, std::ignore)  = names->getBBNames(*join_block);

      eh_vertices[branch_id] = make_tuple(not_handler_id, handler_on_id, handler_off_id);
      handler_to_branch[handler_on_id] = branch_id;
      handler_to_branch[handler_off_id] = branch_id;
      handler_to_branch[not_handler_id] = branch_id;      
    }
  }
}

bool HandlersPass::runOnFunction(Function &F) {
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *inst = &*I;           
    if (BranchInst *branch = dyn_cast<BranchInst>(inst)) {
      if (branch->isUnconditional()) continue;      
      Value *branch_op = branch->getOperand(0);
      ICmpInst *icmp = dyn_cast<ICmpInst>(branch_op);
      if (!icmp) continue;
      
      Value *icmp_op0 = icmp->getOperand(0);
      Value *icmp_op1 = icmp->getOperand(1);        
      addBranchIfCall(branch, icmp_op0);
      addBranchIfCall(branch, icmp_op1);

      if (LoadInst *load = dyn_cast<LoadInst>(icmp_op0)) {
        MemoryDependenceAnalysis &MDA = getAnalysis<MemoryDependenceAnalysis>();
        MDA.runOnFunction(F);
        MemDepResult res = MDA.getDependency(load);
        Instruction *dependency = res.getInst();
        if (dependency) {
          addBranchIfCall(branch, dependency);
          if (StoreInst *store = dyn_cast<StoreInst>(dependency)) {
            Value *sender = store->getOperand(0);
            addBranchIfCall(branch, sender);
          }
        }
      }
    }

    fillEHVertices(F);
  }
  
  return false;
}

void HandlersPass::getAnalysisUsage(AnalysisUsage &AU) const {
  // AU.addRequiredTransitive<AliasAnalysis>();
  AU.addRequiredTransitive<MemoryDependenceAnalysis>();
  AU.addRequired<BranchSafetyPass>();
  AU.addRequired<NamesPass>();
  AU.setPreservesAll();
}

char HandlersPass::ID = 0;
static RegisterPass<HandlersPass> X("handlers", "Identify error handling branches", false, false);
