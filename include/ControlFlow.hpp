#ifndef CONTROLFLOW_HPP
#define CONTROLFLOW_HPP

#include "Names.hpp"
#include "Location.hpp"
#include "FlowGraph.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include <unordered_map>

class ControlFlowPass : public llvm::ModulePass {
public:
  ControlFlowPass() : ModulePass(ID) {}

  static char ID;

  virtual bool runOnModule(llvm::Module &M) override;
  void runOnFunction(llvm::Function *f);
  void runOnBasicBlock(llvm::BasicBlock *bb);
  FlowVertex visitInstruction(llvm::Instruction *I, FlowVertex prev);
  void addMayReturnEdges(llvm::Function &F);
  void addCalls(flow_vertex_t call, FlowVertex ret_v);
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  NamesPass *names;
  FlowGraph FG;

  // Get or generate an id for this value
  void addPredecessorRules(llvm::Instruction*);

  // TODO make const
  void write_dot(const std::string path);

  flow_vertex_t getFunctionVertex(const llvm::Function *F) const;

  bool remove_cross_folder = false;

private:
  unsigned id_cnt = 1;

  // Map from function to the vertex holding the return instruction
  // This is populated in runOnFunction and consumed by addMayReturnEdges
  std::map<llvm::Function*, flow_vertex_t> fn2ret;

  // Map from function to its entry vertex descriptor
  // Populated in runOnModule
  std::map<const llvm::Function*, flow_vertex_t> fn2vtx;
};

#endif
