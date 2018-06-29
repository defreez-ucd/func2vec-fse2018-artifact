#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "Location.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DebugInfo.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <iostream>
#include <stack>
#include <map>

namespace p2v {
  std::unordered_set<std::string> read_interesting_functions(std::string path);
  std::string getName(llvm::Function &F);
};

namespace ep {

enum class TriVal { T, F, N };

Location getSource(llvm::Instruction*);

// Used by clients of DepthFirstVisitor to customize actions during DFS
template<class GraphT>
class DFSVisitorInterface {
  typedef typename boost::graph_traits<GraphT>::vertex_descriptor vertex_t;
  typedef typename boost::graph_traits<GraphT>::edge_descriptor   edge_t;

public:
  virtual void discover_vertex(vertex_t vtx, GraphT &G) = 0;
  virtual bool follow_edge(const edge_t edge, const GraphT &G) const = 0;
};

// BOOST dfs_visit cannot mutate the graph.
// We commonly modify internal properties, so we relax that restriction here.
template<class GraphT>
class DepthFirstVisitor {
  typedef typename boost::graph_traits<GraphT>::vertex_descriptor vertex_t;
  typedef typename boost::graph_traits<GraphT>::edge_descriptor   edge_t;
  typedef typename boost::graph_traits<GraphT>::out_edge_iterator oei_t;

public:
  DepthFirstVisitor(DFSVisitorInterface<GraphT> &visitor) : visitor(visitor) {}

  // Discover all nodes reachable from start vertex
  void visit(vertex_t start_vtx, GraphT &G) {
    std::map<vertex_t, bool> visited;
    std::stack<vertex_t> pending;
    pending.push(start_vtx);

    oei_t oei, oei_end;
    while (!pending.empty()) {
      vertex_t next = pending.top();
      pending.pop();

      if (visited.find(next) == visited.end()) {
        visitor.discover_vertex(next, G);
      }

      visited[next] = true;

      for (std::tie(oei, oei_end) = boost::out_edges(next, G); oei != oei_end; ++oei) {
        vertex_t v = boost::target(*oei, G);
        if (visited.find(v) == visited.end() && visitor.follow_edge(*oei, G)) {
          pending.push(v);
        }
      }
    }
  }

  // Discover all nodes between start and end vertex (including start, but not including end)
  // Does not preserve any order of discovery
  // TODO: Convert visited map to something that can do the lookup faster
  void visit(vertex_t start_vtx, vertex_t end_vtx, GraphT &G) {
    if (!start_vtx || !end_vtx) {
      std::cerr << "FATAL ERROR: Cannot visit null vertex\n";
      abort();
    }

    bool success = false;
    std::vector<std::pair<vertex_t, std::set<vertex_t>>> pending_branches;
    std::set<vertex_t> path;
    std::map<vertex_t, bool> visited;
    std::set<vertex_t> between;

    std::function<void()> end_branch_fn = [&path, &success, &between, &pending_branches]() {
      if (success) {
        between.insert(path.begin(), path.end());
      }
      pending_branches.pop_back();
      success = false;
    };

    pending_branches.push_back(std::make_pair(start_vtx, std::set<vertex_t>()));
    
    bool chain_node = false;
    vertex_t next;
    while (!pending_branches.empty() || chain_node) {     
      if (!chain_node) {
        next = std::get<0>(pending_branches.back());
        path = std::get<1>(pending_branches.back());
      }

      bool next_between = between.find(next) != between.end();

      // If we have already seen the vertex, skip if it wasn't successful
      // Even if it was successful, skip if it is in the current path
      if ((visited.find(next) != visited.end() && !next_between) 
          || path.find(next) != path.end()) {
        chain_node = false;
        end_branch_fn();
        continue;
      }

      path.insert(next);
      visited[next] = true;

      if (next == end_vtx || next_between) {
        chain_node = false;
        success = true;
        end_branch_fn();
        continue;
      }

      oei_t oei, oei_end;
      if (follow_degree(next, G) == 0) {
        chain_node = false;
        end_branch_fn();
      } else if (follow_degree(next, G) == 1) {
        chain_node = true;
  
        for (std::tie(oei, oei_end) = boost::out_edges(next, G); oei != oei_end; ++oei) {
          if (visitor.follow_edge(*oei, G)) {
            next = target(*oei, G);
            break;
          }
        }
      } else {
        chain_node = false;
        for (std::tie(oei, oei_end) = boost::out_edges(next, G); oei != oei_end; ++oei) {
          if (visitor.follow_edge(*oei, G)) {
            pending_branches.push_back(std::make_pair(target(*oei, G), path));
          }
        }
      }
    }

    for (vertex_t v : between) {
      visitor.discover_vertex(v, G);
    }
  }  

private:
  unsigned follow_degree(const vertex_t v, const GraphT &G) const {
    unsigned ret = 0;

    oei_t oei, oei_end;
    for (std::tie(oei, oei_end) = boost::out_edges(v, G); oei != oei_end; ++oei) {
      if (visitor.follow_edge(*oei, G)) {
        ++ret;
      }
    }

    return ret;
  }

  DFSVisitorInterface<GraphT> &visitor;
};

}

#endif
