#ifndef ACTIONVISITORS_HPP
#define ACTIONVISITORS_HPP

#include "Item.hpp"
#include "Names.hpp"
#include "FlowGraph.hpp"
#include <unordered_set>
#include <vector>

// Maps LLVM instructions to actions
class ActionMapper {
public:
  virtual std::vector<Item> map(const llvm::Instruction *I, const Location &loc) = 0;
};

class StandardActionMapper : public ActionMapper {
public:
  StandardActionMapper(NamesPass *names, string tactic) : names(names), tactic(tactic) {}
  virtual std::vector<Item> map(const llvm::Instruction *I, const Location &loc);

private:
  NamesPass *names;
  string tactic;
};

template<class GraphT>
class ActionsVisitor : public ep::DFSVisitorInterface<GraphT> {
  typedef typename boost::graph_traits<GraphT>::vertex_descriptor vertex_t;

public:
  ActionsVisitor(ActionMapper &mapper, std::vector<Item> &actions) :
    mapper(mapper), actions(actions) {}

  std::vector<Item> getActions() const { return actions; }

  // This is where actions are created from llvm instructions
  virtual void discover_vertex(vertex_t vtx, GraphT &G) {
    Location loc = boost::get(&FlowVertex::loc, G, vtx);
    llvm::Instruction *I = boost::get(&FlowVertex::I, G, vtx);

    for (Item item : mapper.map(I, loc)) {
      actions.push_back(item);
    }
  }

private:
  ActionMapper &mapper;
  std::vector<Item> &actions;
};

class HandlerActionsVisitor : public ActionsVisitor<_FlowGraph> {
public:
  HandlerActionsVisitor(ActionMapper &mapper,
			std::vector<Item> &actions,
			string stop,
			const std::unordered_set<string> &handlers,
			std::vector<std::pair<std::string, std::string>> &nesting_pairs) :
    ActionsVisitor<_FlowGraph>(mapper, actions),
    stop(stop),
    handlers(handlers),
    nesting_pairs(nesting_pairs)
  {}

  virtual bool follow_edge(const flow_edge_t edge, const _FlowGraph &G) const;
  virtual void discover_vertex(flow_vertex_t vtx, _FlowGraph &G);

private:
  string stop;

  // Keep track of the first stack location we see.
  // This is always the EH for the handler being visited.
  // Prevents reporting self nesting.
  string self_stack;

  // For determining if a vertex is a handler
  const std::unordered_set<string> &handlers;

  // This visitor is responsible for identifying nested EH
  // List of nesting pairs to append to
  std::vector<std::pair<std::string, std::string>> &nesting_pairs;
};

class SkippedActionsVisitor : public ActionsVisitor<_FlowGraph> {
public:
  SkippedActionsVisitor(ActionMapper &mapper, std::vector<Item> &actions, std::string stop) :
    ActionsVisitor<_FlowGraph>(mapper, actions), stop(stop)
  {}

  virtual bool follow_edge(const flow_edge_t edge, const _FlowGraph &G) const;

private:
  std::string stop;
};

class PostActionVisitor : public ActionsVisitor<_FlowGraph> {
public:
  PostActionVisitor(ActionMapper &mapper, std::vector<Item> &actions, const std::set<flow_vertex_t> discovered) :
    ActionsVisitor<_FlowGraph>(mapper, actions), already_discovered(discovered) {}

  virtual bool follow_edge(const flow_edge_t edge, const _FlowGraph &G) const;
private:
  const std::set<flow_vertex_t> already_discovered;
};

typedef boost::reverse_graph<_FlowGraph> ReverseFlowGraph;

// For collecting actions that happen in a function before handler is reached
// Computes actions that are intraprocedurally backward-reachable from error handler
class PreActionVisitor : public ActionsVisitor<_FlowGraph> {
  typedef typename boost::graph_traits<_FlowGraph>::vertex_descriptor vertex_t;
  typedef typename boost::graph_traits<_FlowGraph>::edge_descriptor   edge_t;

public:
  PreActionVisitor(ActionMapper &mapper, std::vector<Item> &actions) :
      ActionsVisitor<_FlowGraph>(mapper, actions) {}

  virtual bool follow_edge(const edge_t edge, const _FlowGraph &G) const;
  virtual void discover_vertex(vertex_t vtx, _FlowGraph &G);

  std::set<vertex_t> get_discovered() { return discovered; }

private:
  std::set<vertex_t> discovered;
};

class TraceVisitor : public ep::DFSVisitorInterface<_FlowGraph> {
public:
  virtual void discover_vertex(flow_vertex_t vtx, _FlowGraph &G);
};

#endif
