#ifndef PATH_HPP
#define PATH_HPP

#include "FlowGraph.hpp"

// TODO: Add call property to vertices and dispense with this.
inline bool is_call(const FlowGraph &FG, flow_vertex_t v) {
  BGL_FORALL_OUTEDGES(v, e, FG.G, _FlowGraph) {
    if (FG.G[e].call) return true;
  }
  return false;
}

class Path {
public:
  // Empty path. Operations on empty path are currently undefined.
  Path() : FG(nullptr) {}
  Path(std::shared_ptr<FlowGraph> FG) : FG(FG) {}
  Path(std::shared_ptr<FlowGraph> FG, std::string prefix) : FG(FG), prefix(prefix) {}

  Path(const Path &other);

  std::vector<flow_vertex_t> output_vertices;

  unsigned calls_in_path() const;
  unsigned size() const;
  bool empty() const;

  // Use this to add a vertex to the path
  void add(flow_vertex_t v);
  void remove(flow_vertex_t v);
  void remove_call(std::string call);

  void reverse();
  
  const std::vector<flow_vertex_t>& get_vertices() const;
  const std::vector<flow_vertex_t>& get_output_vertices() const;
  std::vector<std::string> output();
  std::string get_callsite_sequence_idx() const;

  // Removes nodes from path up to v and unmarks them on graph FG
  void rewind(flow_vertex_t v);
  bool needs_rewind(flow_vertex_t v);

  // Find the first function change and compare.
  // If this matches with the other path, then call / return line up.
  // If either path does not cross into a new function, then they do match.
  bool valid_match(Path &other);

  // What function is being called by vertex v?
  std::string call_name(flow_vertex_t v) const;

  std::string str();

  // Used for RETURN_ tokens
  std::unordered_set<flow_vertex_t> handler_on;
  std::unordered_set<flow_vertex_t> no_handler_on;
  std::string return_str;
    
private:
  std::vector<flow_vertex_t> vertices;
  
  // Prefer pointer over reference because we want to be able to put
  // paths in containers. References are not assignable.
  std::shared_ptr<FlowGraph> FG;

  std::string prefix;  

  bool parents_valid = true;
  std::vector<std::string> parent_sequence;

  bool in_parent_sequence(const std::string &function_name);
  bool is_output_call(flow_vertex_t v);

  inline std::string get_name(flow_vertex_t v) {
    std::string stack = FG->G[v].stack;
    std::string name = stack.substr(0, stack.find("."));
    return name;
  }

  inline bool is_return(flow_vertex_t v);

public:
  // How can get this to be const?
  // Requires keeping parent_sequence up to date somewhere else.
  inline std::vector<std::string> get_parent_sequence() {
    if (parents_valid) {
      return parent_sequence;
    }

    parent_sequence.clear();
    
    for (const flow_vertex_t v : vertices) {
      std::string stack = FG->G[v].stack;
      std::string parent_fn = stack.substr(0, stack.find("."));      
      if (parent_sequence.empty() || parent_sequence.back() != parent_fn) {
        parent_sequence.push_back(parent_fn);
      }
    }

    parents_valid = true;

    return parent_sequence;
  }
};

#endif
