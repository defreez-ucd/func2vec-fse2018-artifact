#include "TraceVisitors.hpp"

using namespace std;
using namespace llvm;

vector<Item> StandardActionMapper::map(const llvm::Instruction *I, const Location &loc) {
  vector<Item> ret;

  if (!I) {
    return ret;
  }

  if (isa<CallInst>(I)) {
    const CallInst *call = dyn_cast<CallInst>(I);
    vector<string> callee_names = names->getCalleeNames(*call);

    for (auto n : callee_names) {
      Item i(Item::Type::CALL, loc, n);
      i.tactic = tactic;
      ret.push_back(i);
    }
  } else if (isa<StoreInst>(I)) {
    const StoreInst *store = dyn_cast<StoreInst>(I);
    Value *destination = store->getOperand(1);
    vn_t dest_name = names->getVarName(destination);

    if (GetElementPtrInst *gep_inst = dyn_cast<GetElementPtrInst>(destination)) {
      mem_t approx_name = names->getApproxName(*gep_inst);
      if (approx_name) {
        dest_name = approx_name;
      }
    }

    if (dest_name && (dest_name->name().find("cabs2cil") == string::npos)) {
      // We remove cabs2cil because we don't care about stores to temporaries
      Item i(Item::Type::STORE, loc, dest_name->name());
      i.tactic = tactic;
      ret.push_back(i);
    }
  } else if (isa<LoadInst>(I)) {
    const LoadInst *load = dyn_cast<LoadInst>(I);
    Value *from = load->getOperand(0);
    vn_t from_name = names->getVarName(from);
    if (from_name && (from_name->name().find("cabs2cil") == string::npos)) {
      Item i(Item::Type::LOAD, loc, from_name->name());
      i.tactic = tactic;
      ret.push_back(i);
    }
  }

  return ret;
}

void HandlerActionsVisitor::discover_vertex(flow_vertex_t vtx, _FlowGraph &G) {
  ActionsVisitor::discover_vertex(vtx, G);

  if (self_stack.empty()) {
    self_stack = G[vtx].stack;
  }

  // If we find an error-handler here, it is nested.
  if (G[vtx].stack != self_stack && handlers.find(G[vtx].stack) != handlers.end()) {
    nesting_pairs.push_back(std::make_pair(self_stack, G[vtx].stack));
  }
}

bool HandlerActionsVisitor::follow_edge(const flow_edge_t edge, const _FlowGraph &G) const {
  if (!G[edge].may_ret && !G[edge].call && G[target(edge, G)].stack != stop) {
      return true;
  }
  return false;
}

bool SkippedActionsVisitor::follow_edge(const flow_edge_t edge, const _FlowGraph &G) const {
  if (!G[edge].may_ret && !G[edge].call && G[target(edge, G)].stack != stop) {
      return true;
  }
  return false;
}

bool PostActionVisitor::follow_edge(const flow_edge_t edge, const _FlowGraph &G) const {
  if (G[edge].may_ret || G[edge].call) {
      return false;
  }

  flow_vertex_t next = boost::target(edge, G);
  if (already_discovered.find(next) != already_discovered.end()) {
    return false;
  }

  return true;
}

void PreActionVisitor::discover_vertex(vertex_t vtx, _FlowGraph &G) {
  ActionsVisitor::discover_vertex(vtx, G);
  discovered.insert(vtx);
}

bool PreActionVisitor::follow_edge(const edge_t edge, const _FlowGraph &G) const {
  bool may_ret = boost::get(&FlowEdge::may_ret, G, edge);
  bool call    = boost::get(&FlowEdge::call, G, edge);
  if (!may_ret && !call) {
    return true;
  }
  return false;
}

