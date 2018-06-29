#ifndef KCONTEXT_HPP
#define KCONTEXT_HPP

#include <vector>
#include <stack>
#include <memory>
#include "Path.hpp"
#include "FlowGraph.hpp"
#include "Llvm.hpp"

#ifdef DEBUG
#  define DEBUG_PRINT(x) cerr << x
#else
#  define DEBUG_PRINT(x) do {} while (0)
#endif

struct RunMetrics {
  unsigned long long visit_threshold_hits = 0;
};

typedef std::vector<Path> paths_t;
typedef std::vector<std::vector<std::string> > output_t;
typedef std::stack<flow_edge_t> branches_t;
typedef std::vector<flow_vertex_t> path_t;

output_t k_context(std::shared_ptr<FlowGraph> FG,
                   flow_vertex_t start, unsigned path_length, RunMetrics &metrics,
                   bool arg_callinfo, bool err_annotations, p2v::Llvm &passes, std::string return_str);

paths_t __k_context(std::shared_ptr<FlowGraph> FG, flow_vertex_t start,
                    unsigned path_length, bool forward, RunMetrics &metrics,
                    p2v::Llvm &passes);

// Helper Functions
void call_node(flow_vertex_t v, Path &p, branches_t &B,
               unsigned k, const FlowGraph &FG, bool forward);

std::vector<flow_vertex_t> callers(flow_vertex_t v, const FlowGraph &FG);

void run_k_context_on_file(std::string bitcode_path,
                           std::string interesting_path,
                           std::ostream &o,
                           unsigned path_length,
                           bool arg_callinfo,
                           bool err_annotations,
                           std::string return_str = "DEFAULT",
                           std::string error_codes_path = "");

std::vector<flow_vertex_t> get_call_sites(FlowGraph &FG, const std::unordered_set<std::string> &functions);

void add_caller_paths(const FlowGraph &FG, const Path &p, output_t &path_strings);

void add_err_paths(const FlowGraph &FG, Path &p, output_t &path_strings, p2v::Llvm &passes, std::string return_str);

// Single source shortest path to multiple vertices
// Returns a vector of paths, one path for each vertex in the end vector
std::vector<Path> shortest_paths(std::shared_ptr<FlowGraph> FG,
                                 flow_vertex_t start, std::vector<flow_vertex_t> end);

#endif

