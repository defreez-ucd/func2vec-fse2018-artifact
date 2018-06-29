#ifndef TRACES_HPP
#define TRACES_HPP

#include "FlowGraph.hpp"
#include "Item.hpp"
#include "TraceVisitors.hpp"
#include "Utility.hpp"
#include "BranchSafety.hpp"
#include "Names.hpp"
#include "ControlFlow.hpp"
#include "HandlersPass.hpp"
#include "llvm/Analysis/PostDominators.h"
#include <boost/graph/reverse_graph.hpp>
#include <set>
#include <unordered_set>

class Trace {
public:
  Trace(string stack_id) : stack_id(stack_id) {}

  // Serves as identifier for error handler
  std::string stack_id;

  // The response items in the trace
  std::vector<Item> items;

  // Trace contexts
  std::vector<Item> contexts;

  Location location;

  std::string parent_function;

  std::ostream& raw(std::ostream &OS) const {
    OS << "ID|" << stack_id << " ";
    for (Item c : contexts) {
      c.raw(OS) << " ";
    }
    for (Item i : items) {
      i.raw(OS) << " ";
    }
    return OS;
  }
};

class PreActionTrace : public Trace {
public:
  PreActionTrace(string stack_id) : Trace(stack_id) {}

  std::ostream &raw(std::ostream &OS) const {
    OS << "PRE_INTRA|" << stack_id << " ";
    for (Item c : contexts) {
      c.raw(OS) << " ";
    }
    for (Item i : items) {
      i.raw(OS) << " ";
    }
    return OS;
  };

};

class PostActionTrace : public Trace {
public:
  PostActionTrace(string stack_id) : Trace(stack_id) {}

  std::ostream &raw(std::ostream &OS) const {
    OS << "POST_INTRA|" << stack_id << " ";
    for (Item c : contexts) {
      c.raw(OS) << " ";
    }
    for (Item i : items) {
      i.raw(OS) << " ";
    }
    return OS;
  };

};

class Traces {
public:
  // Uses a DataflowResult (such as from DataflowWali, the "lightweight" analysis)
  Traces(ControlFlowPass *control_flow, std::string db_path,
	 BranchSafetyPass *safety, NamesPass *names, llvm::PostDominatorTree *postdom);

  /// \brief Print the traces in a human readable format
  std::ostream& format(std::ostream &OS) const;

  /// \brief Actually create the traces.
  std::ostream& generate(std::ostream &OS) const;

  /// \brief Read the list of error-handling hints from a file.
  ///
  /// The format of the file is one source location and hint type per line.
  /// The source location format is FILENAME:LINENUMBER.
  /// The hint type must be either "branch" or "block".
  /// A branch hint indicates that the source location is for the conditional branch.
  /// In this case the trace generator will attempt to guess the error handling direction.
  /// This only works for conditionals testing error codes.
  /// A block hint indicates that the source location is inside an error handling block, and therefore
  /// it is not necessary to guess the error handling direction.
  void read_handlers(string preds_path);

  /// \brief Setup routine for the trace generator.
  ///
  /// This must be called after read_predicates and before generate.
  void initialize();

private:
  ControlFlowPass *control_flow;
  FlowGraph &FG;
  std::string db_path;

  BranchSafetyPass *safety;
  NamesPass *names;
  llvm::PostDominatorTree *postdom;

  /// \brief The pre-actions (intraprocedural context) for each error-handler.
  ///
  /// EH stack id -> set of pre-actions
  map<string, PreActionTrace> pre_actions;

  /// \brief The post-actions for each error-handler
  ///
  /// EH stack id -> set of ppost-actions
  /// Post-actions are a superset of error-handler traces that do no stop at postdom
  map<string, PostActionTrace> post_actions;

  /// Set of branch locations that might be error handlers, populated by read_handlers.
  std::set<Location> handler_branch_hints;

  /// Set of source locations known to be inside an error handler, populated by read_handlers.
  std::set<Location> handler_block_hints;

  /// \brief Handler stack name -> Predicate location
  std::map<string, Location> handler2pred;

  /// \brief Handler On stack name -> Handler off stack name
  std::map<string, string> handlers_stop;

  // To give traces IDs
  unsigned id_cnt = 0;

  /// \brief resolveBranch is responsible for making decisions about handler branches
  ///
  /// resolveBranch is where decisions are made about whether this branch will be included.
  /// This function relies on the branch safety pass and is therefore only useful for
  /// branches that follow the form of testing an error code. Branches for NULL checks
  /// should never hit this. If the branch safety pass determines that the error handling
  /// side is one the else direction without an explicit else block, then the handler is skipped.
  ///
  /// This is where the handler2pred and handlers_stop maps are populated.
  void resolveBranch(const FlowVertex &V);

  /// \brief resolveBlock is responsible for looking up the branch associated with a block hint.
  ///
  /// Block hints are source locations that are believed to be in an error handler. This makes
  /// everything easier because it is not necessary to guess the direction of the error path.
  void resolveBlock(const FlowVertex &V);

  Trace collectHandlerActions(flow_vertex_t start, string stop, const HandlersPass *handlerspass);
  std::pair<PreActionTrace, PostActionTrace> collectPrePostActions(flow_vertex_t handler);
};

#endif
