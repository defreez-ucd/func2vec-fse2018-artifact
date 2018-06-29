#include "Path.hpp"
#include "Utility.hpp"
#include <algorithm>

using namespace std;

Path::Path(const Path &other) {
  for (const auto &v : other.get_vertices()) {
    add(v);
  }
}

unsigned Path::calls_in_path() const {
   return output_vertices.size();
}

unsigned Path::size() const {
  return vertices.size();
}

bool Path::empty() const {
  return vertices.empty();
}

void Path::add(flow_vertex_t v) {
  vertices.push_back(v);
  if (is_output_call(v)) {
    output_vertices.push_back(v);
  }
  
  std::string v_name = get_name(v);
  if (parent_sequence.empty() || parent_sequence.back() != v_name) {
    parent_sequence.push_back(v_name);
  }
}

void Path::remove(flow_vertex_t v) {
  vertices.erase(std::remove(vertices.begin(), vertices.end(), v), vertices.end());
  output_vertices.erase(std::remove(output_vertices.begin(), output_vertices.end(), v), output_vertices.end());
}

void Path::remove_call(string call) {
  for (const auto &v : get_output_vertices()) {
    if (call_name(v) == call) {
      remove(v);
    }
  }
}

void Path::reverse() {
  std::reverse(vertices.begin(), vertices.end());
  std::reverse(output_vertices.begin(), output_vertices.end());
}

const vector<flow_vertex_t>& Path::get_vertices() const {
  return vertices;
}

const vector<flow_vertex_t>& Path::get_output_vertices() const {
  return output_vertices;
}

vector<string> Path::output() {
  vector<string> ret;

  bool in_err = false, in_no_err = false;
  
  for (const flow_vertex_t v : vertices) {
    if (is_output_call(v)) {
      string name = prefix + call_name(v);
#ifdef DEBUG
      name = name + " " + FG->G[v].loc.str();
#endif
      ret.push_back(name);
    }

    if (handler_on.find(v) != handler_on.end()) {
      in_err = true;
      in_no_err = false;
    }
    if (no_handler_on.find(v) != no_handler_on.end()) {
      in_no_err = true;
    }

    if (is_return(v) && !return_str.empty()) {
      if (in_err) {
        ret.push_back("RETURN_ERR");
      } else if (in_no_err) {
        ret.push_back("RETURN_NO_ERR");
      } else {
        ret.push_back("RETURN_" + return_str);
      }
    }       
  }
  
  return ret;
}

inline bool Path::is_return(flow_vertex_t v) {
  if (boost::out_degree(v, FG->G) == 0) {
    return true;
  }
  BGL_FORALL_OUTEDGES(v, e, FG->G, _FlowGraph) {
    if (FG->G[e].may_ret) return true;
  }
  return false;
}

string Path::get_callsite_sequence_idx() const {
  string ret;
  for (const flow_vertex_t v : output_vertices) {
    ret += FG->G[v].stack;
  }
  return ret;
}

void Path::rewind(flow_vertex_t v) {
  if (!needs_rewind(v)) {
    return;
  }
    
  while (!vertices.empty() && vertices.back() != v) {
    flow_vertex_t n = vertices.back();
    vertices.pop_back();
    FG->G[n].visited = false;
    
    if (!output_vertices.empty() && output_vertices.back() == n) {
      output_vertices.pop_back();
    }
  }

  if (!vertices.empty()) {
    flow_vertex_t n = vertices.back();
    FG->G[n].visited = false;
  }

  parents_valid = false;
}

bool Path::needs_rewind(flow_vertex_t v) {
  return !vertices.empty() && vertices.back() != v;
}

bool Path::valid_match(Path &other) {
  std::vector<std::string> this_sequence = get_parent_sequence();
  std::vector<std::string> other_sequence = other.get_parent_sequence();

  std::vector<std::string> shorter;
  std::vector<std::string> longer;
  if (this_sequence.size() <= other_sequence.size()) {
    shorter = this_sequence;
    longer  = other_sequence;
  } else {
    shorter = other_sequence;
    longer  = this_sequence;
  }
       
  for (unsigned i = 0; i < shorter.size(); ++i) {
    if (shorter[i] != longer[i]) {
      return false;
    }
  }
  return true;
}

bool Path::in_parent_sequence(const std::string &function_name) {
  std::vector<std::string> sequence = get_parent_sequence();
  return std::find(sequence.begin(), sequence.end(), function_name) != sequence.end();
}

bool Path::is_output_call(flow_vertex_t v) {
  if (!is_call(*FG, v)) {
    return false;
  }

  // Hide calls for functions that we are coming out of
  // (interprocedural paths)
  BGL_FORALL_OUTEDGES(v, e, FG->G, _FlowGraph) {
    if (FG->G[e].call) {
      FlowVertex callee = FG->G[target(e, FG->G)];
      std::string name = callee.stack.substr(0, callee.stack.find("."));
      if (in_parent_sequence(name)) {
        return false;
      }
    }
  }
  
  return true;
}

string Path::call_name(flow_vertex_t v) const {
  std::string name;
  
  // Indirect call
  mem_t mem_index = FG->G[v].mem_index;
  if (mem_index) {
    name = mem_index->name();
    replace(name.begin(), name.end(), '.', '_');
    return name;
  }
    
  BGL_FORALL_OUTEDGES(v, e, FG->G, _FlowGraph) {
    if (FG->G[e].call) {
      FlowVertex callee = FG->G[target(e, FG->G)];
      return callee.stack.substr(0, callee.stack.find("."));
    }
  }

  return name;
}

string Path::str() {
  string ret;
  for (const auto &s : output()) {
    ret += s + " ";
  }
  return ret;
}
