#include "Context.hpp"
#include <boost/graph/graph_utility.hpp>
#include <boost/progress.hpp>

using namespace std;
using namespace llvm;
using namespace p2v;

// TODO: This list of parameters is getting out of hand. Make this a class already.
void run_k_context_on_file(string bitcode_path, string interesting_path,
                           ostream &o, unsigned path_length, bool arg_callinfo,
                           bool err_annotations, string return_str, string error_codes_path) {
  RunMetrics metrics;
  unordered_set<string> interesting = read_interesting_functions(interesting_path);
  Llvm passes(bitcode_path, error_codes_path);
  shared_ptr<FlowGraph> FG = passes.getFlowGraph();

  vector<flow_vertex_t> call_sites = get_call_sites(*FG, interesting);  

  cerr << "Generating paths..." << endl;
  boost::progress_display show_progress(num_vertices(FG->G), cerr);
  BGL_FORALL_VERTICES(v, FG->G, _FlowGraph) {
    ++show_progress;
    BGL_FORALL_OUTEDGES(v, e, FG->G, _FlowGraph) {
      if (!FG->G[e].call) continue;
      string name = FG->G[target(e, FG->G)].stack;
      name = name.substr(0, name.find("."));
      if (interesting.find(name) == interesting.end()) continue;
      
      output_t paths = k_context(FG, v, path_length, metrics, arg_callinfo, err_annotations, passes, return_str);
      for (const vector<string>& path : paths) {
        assert(!path.empty());
        
        o << "PATH_BEGIN ";
        for (const string& s : path) o << s << " ";
        o << "PATH_END\n";
      }
    }
  }

  cerr << endl << "Metrics" << endl
       << "======" << endl
       << "Visit threshold hits: " << metrics.visit_threshold_hits << endl;
}

// Return the list of may return sites for the parent function of this node
vector<flow_vertex_t> callers(flow_vertex_t v, const FlowGraph &FG) {
  vector<flow_vertex_t> ret;
  
  string name = FG.G[v].stack;
  name = name.substr(0, name.find("."));
  name += ".0";

  flow_vertex_t fn_entry = FG.stack_vertex_map.at(name);

  BGL_FORALL_INEDGES(fn_entry, e, FG.G, _FlowGraph) {
    if (FG.G[e].call) {
      flow_vertex_t call_site = source(e, FG.G);
      BGL_FORALL_OUTEDGES(call_site, ret_e, FG.G, _FlowGraph) {
        if (FG.G[ret_e].ret) {
          ret.push_back(target(ret_e, FG.G));
        }
      }
    }
  }
  
  return ret;
}

output_t k_context(shared_ptr<FlowGraph> FG, flow_vertex_t start,
                   unsigned path_length, RunMetrics &metrics, bool arg_callinfo,
                   bool err_annotations, Llvm &passes, string return_str) {
  paths_t forward  = __k_context(FG, start, path_length, true, metrics, passes);
  paths_t backward = __k_context(FG, start, path_length, false, metrics, passes);
  
  unordered_set<string> seen_callsite_sequences;
  
  output_t ret;
  for (Path &f : forward) {
    vector<string> f_out = f.output();
    if (f_out.size() == 0) continue;
    
    for (Path &b : backward) {            
      if (!f.valid_match(b)) continue;

      string callsites = f.get_callsite_sequence_idx() + b.get_callsite_sequence_idx();
      if (seen_callsite_sequences.find(callsites) != seen_callsite_sequences.end()) {
        continue;
      } else {
        seen_callsite_sequences.insert(callsites);
      }
      
      vector<string> combined = b.output();      
      if (combined.size() == 0) continue;

      Path forward_back(b);
      flow_vertex_t redundant = forward_back.get_vertices()[0];
      forward_back.remove(redundant);
      forward_back.reverse();            
      for (const auto &v : f.get_vertices()) {
        forward_back.add(v);
      }
           
      if (arg_callinfo) {
        add_caller_paths(*FG, forward_back, ret);
      }
      if (err_annotations) {
        add_err_paths(*FG, forward_back, ret, passes, return_str);
      }

      if (! forward_back.output().empty()) {
        ret.push_back(forward_back.output());
      }
    }
  }

  return ret;
}

// Adds CALLER_ paths to path_list
void add_caller_paths(const FlowGraph &FG, const Path &p, output_t &path_strings) {
  for (const flow_vertex_t v : p.get_output_vertices()) {
    vector<string> caller_path;
    string stack = FG.G[v].stack;
    string fname = stack.substr(0, stack.find('.'));
    string call = "CALLER_" + fname;
    caller_path.push_back(call);
    caller_path.push_back(p.call_name(v));
    path_strings.push_back(caller_path);
  }
}

// Note that this mutates the path p to remove failed functions
void add_err_paths(const FlowGraph &FG, Path &p, output_t &path_strings, Llvm &passes, string return_str) {
  unordered_set<string> handlers_on, handlers_off, handlers_not;
  handlers_on = passes.handlers_on;
  handlers_off = passes.handlers_off;
  handlers_not = passes.handlers_not;

  // Turn on/off returns (which only make sense when error path annotations are enabled)
  p.return_str = return_str;

  DEBUG_PRINT("Full Path is: " + p.str() + "\n");

  vector<string> err_path_for;
  vector<string> no_err_path_for;
  bool empty_err = true;

  vector<flow_vertex_t> path_vertices = p.get_vertices();

  // Go through once and remove the error path vertices
  // This makes it much easier to keep track of things later
  // because we don't know if we should include a call in a branch until we see
  // which direction the path went.
  for (const flow_vertex_t v : path_vertices) {
    string call_name = p.call_name(v);
    string id = FG.G[v].stack;
    if (handlers_on.find(id) != handlers_on.end()) {
      string branch = passes.handler_to_branch[id];
      string e = passes.returning_functions_by_id[branch];
      p.remove_call(e);
    } 
  }

  if (p.get_output_vertices().empty()) return;

  DEBUG_PRINT("Mutated Path is: " + p.str() + "\n");

  path_vertices = p.get_vertices();
  for (const flow_vertex_t v : path_vertices) {
    string call_name = p.call_name(v);
    string id = FG.G[v].stack;

    if (!call_name.empty()) {
      DEBUG_PRINT("Processing: " + call_name + "\n");
    } else {
      DEBUG_PRINT("Processing: " + id + "\n");
    }
    
    if (handlers_on.find(id) != handlers_on.end()) {      
      string branch = passes.handler_to_branch[id];
      string fn = passes.returning_functions_by_id[branch];
      if (!fn.empty()) {
        err_path_for.push_back(passes.returning_functions_by_id[branch]);        
      }
      p.handler_on.insert(v);
    }
    if (handlers_not.find(id) != handlers_not.end()) {
      string branch = passes.handler_to_branch[id];
      string fn = passes.returning_functions_by_id[branch];
      if (!fn.empty()) {
        no_err_path_for.push_back(passes.returning_functions_by_id[branch]);
      }
      p.no_handler_on.insert(v);
    }
    if (! err_path_for.empty()) {
      if (! call_name.empty()) {
        for (string &err_for : err_path_for) {
          empty_err = false;
          vector<string> err_path;
          string err = "ERR_";
          err += err_for;
          string call = p.call_name(v);
          err_path.push_back(err);
          err_path.push_back(call);
          path_strings.push_back(err_path);
        }
      }      
    }
    for (string &no_err_for : no_err_path_for) {
      vector<string> no_err_path;
      bool call_is_err   = std::find(err_path_for.begin(), err_path_for.end(), call_name) != err_path_for.end();
      bool no_err_is_err = std::find(err_path_for.begin(), err_path_for.end(), no_err_for) != err_path_for.end();
      
      if (!call_name.empty() && !call_is_err && !no_err_is_err) {
        string no_err = "NO_ERR_";
        no_err += no_err_for;
        no_err_path.push_back(no_err);
        no_err_path.push_back(call_name);
        path_strings.push_back(no_err_path);
        DEBUG_PRINT("Add NO_ERR_" + no_err_for + " " + call_name + "\n");
      }    
    }
  }

  if (empty_err) {
    for (string &err_for : err_path_for) {
      vector<string> err_path;
      string err = "ERR_";
      err += err_for;
      err_path.push_back(err);
      path_strings.push_back(err_path);      
    }
  }

  DEBUG_PRINT("End path is " + p.str() + "\n");
}

paths_t __k_context(shared_ptr<FlowGraph> FG, flow_vertex_t start,
                    unsigned path_length, bool forward, RunMetrics &metrics,
                    Llvm &passes) {
  paths_t path_list; 
  branches_t B;
  Path path(FG);
 
  unordered_set<string> seen_callsite_sequences;
  
  if (!is_call(*FG, start)) {
    return path_list;
  }
   
  path.add(start);
  FG->G[start].visited = true;

  DEBUG_PRINT("__k_context " << forward << " " << FG->G[start].stack << endl);

  unordered_set<string> on, off;
  if (forward) {
    on = passes.handlers_on;
    off = passes.handlers_off;
  } else {
    on = passes.handlers_off;
    off = passes.handlers_on;
  }

  if (forward) {
    BGL_FORALL_OUTEDGES(start, e, FG->G, _FlowGraph) {
      if (!FG->G[e].call) {
        B.push(e);
      }
    }
  } else {
    BGL_FORALL_INEDGES(start, e, FG->G, _FlowGraph) {
      if (!FG->G[e].may_ret) {
        B.push(e);
      }
    }
  }

  unsigned iterations = 0;
  while (! B.empty()) {
    DEBUG_PRINT("---" << endl << iterations << endl);
    iterations++;
    flow_edge_t b = B.top();
    B.pop();

    // Thresholds for early exit
    // -------------------------
    if (path.size() > path_length) {
      // Path is getting too long. Let's search elsewhere.
      continue;
    }
    if (iterations > 1000 * path_length) {
      // Bail out on this call site entirely.
      metrics.visit_threshold_hits += 1;
      break;
    }

    flow_vertex_t tail;
    if (forward) {
      tail = source(b, FG->G);
    } else {
      tail = target(b, FG->G);
    }

    // We finished a path.
    // The branch stack is behind the tip of the path.
    // Remove the end of the path up to next branch and unmark nodes.
    if (path.needs_rewind(tail)) {
      string callsites = path.get_callsite_sequence_idx();
      if (seen_callsite_sequences.find(callsites) == seen_callsite_sequences.end()) {
        seen_callsite_sequences.insert(callsites);
        path_list.push_back(path);      
      }

      DEBUG_PRINT("rewind " << FG->G[tail].stack << endl);
      path.rewind(tail);
    }

    flow_vertex_t next;
    if (forward) {
      next = target(b, FG->G);
    } else {
      next = source(b, FG->G);
    } 

    if (FG->G[next].visited) {
      DEBUG_PRINT("already visited" << endl);
      continue;
    }

    path.add(next);
    FG->G[next].visited = true;
    DEBUG_PRINT(FG->G[next].stack << endl);

    if (forward) {
      BGL_FORALL_OUTEDGES(next, e, FG->G, _FlowGraph) {
        if (!FG->G[e].call) {
          B.push(e);
        }
      }
    } else  {
      BGL_FORALL_INEDGES(next, e, FG->G, _FlowGraph) {
        if (!FG->G[e].may_ret) {
          B.push(e);
        }
      }
    }
  }

  // Append last path
  if (!path.empty()) {
    for (const flow_vertex_t v : path.get_vertices()) {
      FG->G[v].visited = false;
    }

    string callsites = path.get_callsite_sequence_idx();
    if (seen_callsite_sequences.find(callsites) == seen_callsite_sequences.end()) {
      path_list.push_back(path);
    }
  }

  return path_list;
}

vector<flow_vertex_t> get_call_sites(FlowGraph &FG, const unordered_set<string> &functions) {
  unordered_set<flow_vertex_t> unique_sites;

  BGL_FORALL_VERTICES(v, FG.G, _FlowGraph) {
    BGL_FORALL_OUTEDGES(v, e, FG.G, _FlowGraph) {
      if (!FG.G[e].call) continue;
      string name = FG.G[target(e, FG.G)].stack;
      name = name.substr(0, name.find("."));
      if (functions.find(name) != functions.end()) {
        unique_sites.insert(v);
      }
    }
  }

  vector<flow_vertex_t> ret;
  copy(unique_sites.begin(), unique_sites.end(), back_inserter(ret));
  return ret;
}
