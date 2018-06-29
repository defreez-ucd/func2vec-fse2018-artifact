#include "InstructionLabels.hpp"
#include "Names.hpp"
#include <iostream>

using namespace std;
using namespace llvm;

char InstructionLabelsPass::ID = 0;
static llvm::RegisterPass<InstructionLabelsPass> X("instruction-labels",
                                                   "Add per-instruction labels to the flowgraph", false, false);

bool InstructionLabelsPass::runOnModule(Module &M) {
  LabelVisitor LV(&getAnalysis<NamesPass>(), (&getAnalysis<ControlFlowPass>())->FG);
  LV.visit(M);

  label_to_id = LV.label_to_id;

  return false;
}

void LabelVisitor::visitInstruction(llvm::Instruction &I) {
  if (isa<CallInst>(I)) {
    return;
  }

  flow_vertex_t vertex = FG.stack_vertex_map[names->getStackName(I)];
  if (!vertex) return;
  string label = "F2V_INST_";
  label += I.getOpcodeName();
  FG.G[vertex].label_ids.push_back(getOrCreateId(label));
}


void LabelVisitor::visitBranchInst(llvm::BranchInst &I) {
  if (I.isUnconditional()) {
    return;
  }

  flow_vertex_t vertex = FG.stack_vertex_map[names->getStackName(I)];
  FG.G[vertex].label_ids.push_back(getOrCreateId("F2V_CONDBR"));
}


void LabelVisitor::visitStoreInst(llvm::StoreInst &I) {
  if (I.getNumOperands() == 0) {
    return;
  }

  Value *sender = I.getOperand(0)->stripPointerCasts();
  vn_t sender_name = names->getVarName(sender);
  flow_vertex_t vertex = FG.stack_vertex_map[names->getStackName(I)];
  if (sender_name && sender_name->type == VarType::EC && sender_name->name() != "OK") {
    FG.G[vertex].label_ids.push_back(getOrCreateId("F2V_ERR_" + sender_name->name()));
  } else {
    FG.G[vertex].label_ids.push_back(getOrCreateId("F2V_INST_store"));
  }
}

void LabelVisitor::visitReturnInst(llvm::ReturnInst &I) {
  if (I.getNumOperands() == 0) {
    return;
  }

  Value *ret = I.getOperand(0)->stripPointerCasts();
  vn_t ret_name = names->getVarName(ret);
  if (ret_name && ret_name->type == VarType::EC && ret_name->name() != "OK") {
    flow_vertex_t vertex = FG.stack_vertex_map[names->getStackName(I)];
    FG.G[vertex].label_ids.push_back(getOrCreateId(ret_name->name()));
  }
}

void LabelVisitor::visitGetElementPtrInst(llvm::GetElementPtrInst &I) {
  flow_vertex_t vertex = FG.stack_vertex_map[names->getStackName(I)];
  if (I.getNumOperands() < 3) {
    FG.G[vertex].label_ids.push_back(getOrCreateId("F2V_INST_getelementptr"));
    return;
  }
  vn_t approx_name = names->getApproxName(I);
  if (approx_name && approx_name->type != VarType::MULTI) {
    string type_name = approx_name->name();
    type_name = type_name.substr(type_name.find(".")+1);
    type_name = type_name.substr(0, type_name.find("."));
    string label = "F2V_GEP_" + type_name;
    FG.G[vertex].label_ids.push_back(getOrCreateId(label));
  } else {
    FG.G[vertex].label_ids.push_back(getOrCreateId("F2V_INST_getelementptr"));
  }
}

int LabelVisitor::getOrCreateId(const string &label) {
  auto it = label_to_id.find(label);
  if (it != label_to_id.end()) {
    return it->second;
  }
  const int id = label_to_id.size();
  bool ok1 = label_to_id.insert(std::make_pair(label, id)).second;
  assert(ok1);
  return id;
}


void InstructionLabelsPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<NamesPass>();
  AU.addRequired<ControlFlowPass>();
}