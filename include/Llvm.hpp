#ifndef PROGRAM2VEC_LLVM_HPP
#define PROGRAM2VEC_LLVM_HPP

#include "FlowGraph.hpp"
#include "Names.hpp"
#include "ControlFlow.hpp"
#include "ReturnValuesPass.hpp"
#include "InstructionLabels.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include <map>
#include <string>
#include <memory>

namespace p2v {

  class Llvm {
  public:
    Llvm(string bitcode_path, string error_codes_path="", bool remove_cross_folder=false);

    // Get a pointer to the FlowGraph.
    std::shared_ptr<FlowGraph> getFlowGraph() const;

    // Get a copy of the bootstrap function sets.
    std::map<std::string, std::set<string>> getBootstrapFns() const;

    std::tuple<std::string, std::string, std::string> getEHVertices(std::string) const;

    // Sloppy
    std::map<std::string, std::tuple<std::string, std::string, std::string>> eh_vertices;
    std::unordered_set<std::string> handlers_not;
    std::unordered_set<std::string> handlers_on;
    std::unordered_set<std::string> handlers_off;
    std::map<std::string, std::string> returning_functions_by_id;
    std::map<std::string, std::string> handler_to_branch;
    std::unordered_map<int, std::string> id_to_label;

  private:
    // FlowGraph is heap allocated in constructor. Shared ownership so
    // that FlowGraph can still be used after passes object passes out of scope.
    std::shared_ptr<FlowGraph> FG;

    std::map<std::string, std::set<string>> bootstrap_fns;
  };
}

#endif
