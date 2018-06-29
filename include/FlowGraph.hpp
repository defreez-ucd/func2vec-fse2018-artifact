#ifndef FLOWGRAPH_H
#define FLOWGRAPH_H

#include "Utility.hpp"
#include "Location.hpp"
#include "VarName.hpp"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/BasicBlock.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <sstream>
#include <algorithm>
#include <tuple>

struct FlowVertex;
struct FlowEdge;

typedef boost::adjacency_list<boost::setS, boost::setS, boost::bidirectionalS,
  FlowVertex, FlowEdge> _FlowGraph;

typedef boost::graph_traits<_FlowGraph>::vertex_descriptor  flow_vertex_t;
typedef boost::graph_traits<_FlowGraph>::edge_descriptor    flow_edge_t;
typedef boost::graph_traits<_FlowGraph>::edge_iterator      flow_edge_iter;
typedef boost::graph_traits<_FlowGraph>::out_edge_iterator  flow_out_edge_iter;
typedef boost::graph_traits<_FlowGraph>::in_edge_iterator   flow_in_edge_iter;
typedef boost::graph_traits<_FlowGraph>::vertex_iterator    flow_vertex_iter;


struct FlowVertex {
  // Boost needs default constructor
  FlowVertex() {}

  FlowVertex(std::string stack, llvm::Function *F) :
    stack(stack), F(F) {}

  // I should be nullptr only in unit tests
  FlowVertex(std::string stack, Location loc, llvm::Instruction *I) :
      stack(stack), loc(loc), I(I) {

    if (I) {
      F = I->getParent()->getParent();
    }
  }

  FlowVertex(unsigned stack, Location loc, llvm::Instruction *I) :
    FlowVertex(std::to_string(stack), loc, I) {}

  std::string stack;
  Location loc;
  llvm::Instruction *I = nullptr;

  // This should always be set when creating vertices unless vertex is completely empty
  llvm::Function* F = nullptr;

  bool visited = false;
  boost::default_color_type color;

  // func2vec
  // ==========
  mem_t mem_index = nullptr;    // populated for indirect calls
  //std::string error_code;       // Any error code we saw in a return / store instruction
  std::vector<int> label_ids;
};

struct FlowEdge {
  bool call = false;

  // Edge from call vertex to instruction after function returns
  bool ret  = false;

  // Edge from return instruction to *possible* return location
  bool may_ret = false;

  // Edges from main.0 when there is no main function
  bool main = false;
};

// Templatized so filtered graphs can be written out
template <typename GraphTy>
class vertex_writer {
public:
  vertex_writer(GraphTy &G) : _G(G) {}
  template <class VertexOrEdge>
  void operator()(std::ostream& os, const VertexOrEdge &v) const {
    if (_G[v].loc.line == 0) {
      os << "[label=\"" << _G[v].stack << "\"]";
    } else {
      os << "[label=\"" << _G[v].stack << " (" << _G[v].loc << ")\"]";
    }
  }
private:
  const GraphTy &_G;
};

template <typename GraphTy>
class call_writer {
public:
  call_writer(GraphTy &G) : _G(G) {}
  template <class VertexOrEdge>
  void operator()(std::ostream& os, const VertexOrEdge &e) const {
    if (_G[e].call) {
      os << "[label=\"call\"]";
    } else if (_G[e].ret) {
      os << "[label=\"ret\"]";
    } else if (_G[e].may_ret) {
      os << "[label=\"may_ret\"]";
    } else if (_G[e].main) {
      os << "[label=\"main\"]";
    }
  }
private:
  const GraphTy &_G;
};

class edge_visited_predicate {
public:
  // Empty constructor because boost
  edge_visited_predicate() {}
  edge_visited_predicate(_FlowGraph &G) : _G(&G) {}

  bool operator()(const flow_edge_t E) const {
    return (*_G)[source(E, *_G)].visited && (*_G)[target(E, *_G)].visited;
  }
private:
  _FlowGraph *_G;
};

class vertex_visited_predicate {
public:
  vertex_visited_predicate() {}
  vertex_visited_predicate(_FlowGraph &G) : _G(&G) {}

  bool operator()(const flow_vertex_t V) const {
    return (*_G)[V].visited;
  }
private:
  _FlowGraph *_G;
};

// Used by write_graphviz to mark vertices visited
class GraphvizVisitor : public ep::DFSVisitorInterface<_FlowGraph> {
public:
  virtual void discover_vertex(flow_vertex_t vtx, _FlowGraph &G) {
    G[vtx].visited = true;
  }

  virtual bool follow_edge(const flow_edge_t edge, const _FlowGraph &G) const {
    if (!G[edge].may_ret && !G[edge].call) {
      return true;
    }
    return false;
  }
};

class FlowGraph {
public:
  // Return value for add
  // from, to1, to2, edge1, edge2
  typedef std::tuple<flow_vertex_t, flow_vertex_t, flow_vertex_t, flow_edge_t, flow_edge_t> add_t;

  // TODO: Total rewrite of add(...).
  // TODO: This was a terrible design, but they are called in many places.
  add_t add(FlowVertex from) {
    FlowVertex empty1;
    FlowVertex empty2;
    return add(from, empty1, empty2);
  }

  add_t add(FlowVertex from, FlowVertex to) {
    FlowVertex empty;
    return add(from, to, empty);
  }

  // The other add methods are just wrappers around this.
  add_t add(FlowVertex from, FlowVertex to1, FlowVertex to2) {
    if (from.stack.empty()) abort();

    flow_vertex_t vertex_to1 = nullptr, vertex_to2 = nullptr;
    flow_vertex_t vertex_from = find_or_add_vertex(from.stack);
    flow_edge_t edge1, edge2;
    G[vertex_from] = from;

    if (!to1.stack.empty()) {
      vertex_to1 = find_or_add_vertex(to1.stack);
      G[vertex_to1] = to1;
      tie(edge1, std::ignore) = boost::add_edge(vertex_from, vertex_to1, G);
    }

    if (!to2.stack.empty()) {
      vertex_to2 = find_or_add_vertex(to2.stack);
      G[vertex_to2] = to2;
      tie(edge2, std::ignore) = boost::add_edge(vertex_from, vertex_to2, G);
      G[edge1].call = true;
      G[edge2].ret  = true;
    }

    return std::make_tuple(vertex_from, vertex_to1, vertex_to2, edge1, edge2);
  }

  flow_vertex_t find_or_add_vertex(std::string stack) {
    flow_vertex_t v;

    if (stack_vertex_map.find(stack) != stack_vertex_map.end()) {
      v = stack_vertex_map[stack];
    } else {
      v = boost::add_vertex(G);
      stack_vertex_map[stack] = v;
    }
    G[v].stack = stack;

    if (stack == "main.0") {
      entry = v;
    }

    return v;
  }

  flow_vertex_t get_entry() {
    return entry;
  }

  // Lookup the vertex descript for this stack location
  // Returns nullptr if that stack does not exist in the FlowGraph
  flow_vertex_t getVertex(std::string stack) {
    if (stack_vertex_map.find(stack) == stack_vertex_map.end()) {
      return nullptr;
    }

    return stack_vertex_map.find(stack)->second;
  }

  void write_graphviz(std::ostream& os) {
    // Need a index map because we are using setS for vertex list
    std::map<flow_vertex_t, size_t> index_map;
    for (auto u : boost::make_iterator_range(boost::vertices(G))) {
      index_map[u] = index_map.size();
    }

    boost::default_writer w;
    call_writer<_FlowGraph> cw(G);
    vertex_writer<_FlowGraph> vw(G);
    boost::write_graphviz(
      os, G,
      vw,
      cw,
      w,
      boost::make_assoc_property_map(index_map)
    );
  }

  void write_graphviz(std::ostream& os, std::string stack_start) {
    for (flow_vertex_t v : boost::make_iterator_range(boost::vertices(G))) {
      G[v].visited = false;
    }

    if (stack_vertex_map.find(stack_start) == stack_vertex_map.end()) {
      std::cerr << "FATAL ERROR: Filtered dot requested for unknown function\n";
      abort();
    }

    // Mark all of the vertices that are reachable from start_stack
    GraphvizVisitor gv;
    ep::DepthFirstVisitor<_FlowGraph> visitor(gv);
    visitor.visit(stack_vertex_map[stack_start], G);

    // Need a index map because we are using setS for vertex list
    std::map<flow_vertex_t, size_t> index_map;
    for (auto u : boost::make_iterator_range(boost::vertices(G))) {
      index_map[u] = index_map.size();
    }

    edge_visited_predicate evp(G);
    vertex_visited_predicate vvp(G);

    typedef boost::filtered_graph<_FlowGraph, edge_visited_predicate, vertex_visited_predicate> FilteredTy;
    FilteredTy FilteredG(G, evp, vvp);

    boost::default_writer w;
    call_writer<FilteredTy> cw(FilteredG);
    vertex_writer<FilteredTy> vw(FilteredG);
    boost::write_graphviz(
      os, FilteredG,
      vw,
      cw,
      w,
      boost::make_assoc_property_map(index_map)
    );
  }

  _FlowGraph G;

  std::map<std::string, flow_vertex_t> stack_vertex_map;

private:
  flow_vertex_t entry;
};

#endif
