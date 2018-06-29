

#include "FlowGraph.hpp"
#include "Location.hpp"
#include "Utility.hpp"
#include "TraceDatabase.hpp"
#include <llvm/IR/Instructions.h>
#include <iostream>
#include <stack>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace ep;
using namespace llvm;

Traces::Traces(ControlFlowPass *control_flow, string db_path,
    BranchSafetyPass *safety, NamesPass *names, PostDominatorTree *postdom) :
      control_flow(control_flow), FG(control_flow->FG),
      db_path(db_path), safety(safety), names(names), postdom(postdom) {}

void Traces::initialize() {
  flow_vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(FG.G); vi != vi_end; ++vi) {
    FlowVertex v = FG.G[*vi];
    if (v.I == NULL) continue;
    if (handler_branch_hints.find(v.loc) != handler_branch_hints.end()) {
      resolveBranch(v);
    } else if (handler_block_hints.find(v.loc) != handler_block_hints.end()) {
      resolveBlock(v);
    }
  }

  for (tie(vi, vi_end) = vertices(FG.G); vi != vi_end; ++vi) {
    FlowVertex vtx = FG.G[*vi];
    auto stop = handlers_stop.find(vtx.stack);
    if (stop != handlers_stop.end()) {
      std::pair<PreActionTrace, PostActionTrace> pre_post = collectPrePostActions(*vi);
      pre_actions.emplace(stop->first, std::get<0>(pre_post));
      post_actions.emplace(stop->first, std::get<1>(pre_post));
    }
  }
}

std::ostream& Traces::generate(std::ostream &OS) const {
  TraceDatabase TD(db_path);
  map<string, sqlite3_int64> handler_row_ids;

  for (const auto &pair : pre_actions) {
    const Trace &t = pair.second;
    sqlite3_int64 handler_id = TD.addHandlerTrace(t);
    handler_row_ids[t.stack_id] = handler_id;

    // Write pre-actions (intra-procedural context) for this handler
    const auto &pre_iter = pre_actions.find(t.stack_id);
    if (pre_iter == pre_actions.end()) {
      abort();
    }
    const PreActionTrace &pre_trace = pre_iter->second;
    TD.addPreActionTrace(handler_id, pre_trace);

    // Write post-actions for this handler
    const auto &post_iter = post_actions.find(t.stack_id);
    if (post_iter == post_actions.end()) {
      abort();
    }
    const PostActionTrace &post_trace = post_iter->second;
    TD.addPostActionTrace(handler_id, post_trace);
  }

  return OS;
}

std::pair<PreActionTrace, PostActionTrace> Traces::collectPrePostActions(flow_vertex_t handler) {
  llvm::Function *F = FG.G[handler].F;
  StandardActionMapper pre_mapper(names, "PRE");
  StandardActionMapper post_mapper(names, "POST");

  string handler_stack = boost::get(&FlowVertex::stack, FG.G, handler);

  PreActionTrace pre_trace(handler_stack);
  PreActionVisitor pre_vis(pre_mapper, pre_trace.contexts);
  DepthFirstVisitor<_FlowGraph> pre_dfs(pre_vis);
  flow_vertex_t fnVertex = control_flow->getFunctionVertex(F);
  pre_trace.location = handler2pred[handler_stack];
  pre_trace.parent_function = handler_stack.substr(0, handler_stack.find('.'));

  pre_dfs.visit(fnVertex, handler, FG.G);

  FlowVertex start_vtx = FG.G[handler];

  PostActionTrace post_trace(handler_stack);
  PostActionVisitor post_vis(post_mapper, post_trace.items, pre_vis.get_discovered());
  DepthFirstVisitor<_FlowGraph> post_dfs(post_vis);
  post_dfs.visit(handler, FG.G);

  return std::make_pair(pre_trace, post_trace);
}

void Traces::resolveBlock(const FlowVertex &V) {
  Instruction *inst = V.I;
  assert(V.I);
  BasicBlock *handler_block = inst->getParent();

  for (auto it = pred_begin(handler_block), et = pred_end(handler_block); it != et; ++it) {
    BasicBlock *predecessor = *it;
    Instruction *terminator = predecessor->getTerminator();

    if (!isa<BranchInst>(terminator)) continue;
    BranchInst *branch = dyn_cast<BranchInst>(terminator);
    assert(branch);
    if (branch->isUnconditional()) continue;

    BasicBlock *not_handler_block = nullptr;
    if (branch->getSuccessor(0) == handler_block) {
      not_handler_block = branch->getSuccessor(1);
    } else {
      not_handler_block = branch->getSuccessor(0);
    }

    // Handler is the empty else branch - do nothing
    postdom->runOnFunction(*handler_block->getParent());
    if (postdom->dominates(handler_block, not_handler_block)) return;

    // Find where control flow merges again
    BasicBlock *join_block = postdom->findNearestCommonDominator(handler_block, not_handler_block);
    if (!join_block) continue;

    DILocation *loc = terminator->getDebugLoc();
    if (!loc) continue;

    unsigned line = loc->getLine();
    string file = loc->getFilename();
    string handler_stack;
    tie(handler_stack, std::ignore) = names->getBBNames(*handler_block);
    assert(!handler_stack.empty());
    Location handler_loc(file, line);
    handler2pred[handler_stack] = handler_loc;

    string handler_off_stack;
    tie(handler_off_stack, std::ignore) = names->getBBNames(*join_block);

    handlers_stop[handler_stack] = handler_off_stack;
  }
}


void Traces::resolveBranch(const FlowVertex &V) {
  if (!isa<BranchInst>(V.I)) return;
  BranchInst *branch = dyn_cast<BranchInst>(V.I);

  std::pair<BasicBlock*, BasicBlock*> branch_blocks = safety->getBranchBlocks(branch);

  BasicBlock *handler_block = branch_blocks.second;
  BasicBlock *not_handler_block = branch_blocks.first;

  if (!handler_block) return;

  // Handler is the empty else branch - do nothing
  postdom->runOnFunction(*handler_block->getParent());
  if (postdom->dominates(handler_block, not_handler_block)) return;

  // Find where control flow merges again
  BasicBlock *join_block = postdom->findNearestCommonDominator(handler_block, not_handler_block);

  string handler_stack;
  tie(handler_stack, std::ignore) = names->getBBNames(*handler_block);

  // No common post-dominator. Something funky going on.
  // We have to skip this error-handling block
  if (!join_block) return;

  string handler_off_stack;
  tie(handler_off_stack, std::ignore) = names->getBBNames(*join_block);

  handlers_stop[handler_stack] = handler_off_stack;
  handler2pred[handler_stack] = V.loc;
}

void Traces::read_handlers(string handler_hints_path) {
  ifstream f_hints(handler_hints_path);
  string line;

  while(getline(f_hints, line)) {
    string file_and_line, file_hint, hint_type;
    unsigned line_hint;

    istringstream line_iss(line);
    line_iss >> file_and_line;
    line_iss >> hint_type;

    vector<string> strs;
    boost::split(strs, file_and_line, boost::is_any_of(":"));
    file_hint = strs[0];

    line_hint = boost::lexical_cast<unsigned>(strs[1]);

    if (hint_type == "branch") {
      Location branch_loc(file_hint, line_hint);
      handler_branch_hints.insert(branch_loc);
    } else if (hint_type == "block") {
      Location block_loc(file_hint, line_hint);
      handler_block_hints.insert(block_loc);
    }
  }

  f_hints.close();
}

