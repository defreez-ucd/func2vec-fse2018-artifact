#include "Llvm.hpp"
#include "BranchSafety.hpp"
#include "HandlersPass.hpp"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"

using namespace llvm;

namespace p2v {

  Llvm::Llvm(string bitcode_path, string ec_path, bool remove_cross_folder) {
    SMDiagnostic Err;
    unique_ptr<Module> Mod(parseIRFile(bitcode_path, Err, getGlobalContext()));
    if (!Mod) {
      cerr << "FATAL: Error parsing bitcode file: " << bitcode_path << endl;
      abort();
    }

    // Passes must be allocated with new, but owernship is transferred
    // to the pass manager. We let the pass manager go out of scope, so we need to know
    // ahead of time what we need from the passes.
    // TODO: Only run the passes needed by executable.
    legacy::PassManager PM;
    NamesPass *names = new NamesPass(ec_path);
    ControlFlowPass *cfp = new ControlFlowPass();
    cfp->remove_cross_folder = remove_cross_folder;
    InstructionLabelsPass *ilp = new InstructionLabelsPass();
    PM.add(names);
    PM.add(cfp);
    PM.add(ilp);
    PM.run(*Mod);

    // TODO: move instead of copy
    FG = make_shared<FlowGraph>(cfp->FG);
    bootstrap_fns = names->get_bootstrap_functions();

    for (const auto &kv : ilp->label_to_id) {
      bool ok = id_to_label.insert({kv.second, kv.first}).second;
      assert(ok);
    }
  }

  shared_ptr<FlowGraph> Llvm::getFlowGraph() const {
    return FG;
  }

  map<string, set<string>> Llvm::getBootstrapFns() const {
    return bootstrap_fns;
  }

  tuple<string, string, string> Llvm::getEHVertices(string id) const {
    if (eh_vertices.find(id) != eh_vertices.end()) {
      return eh_vertices.at(id);
    }
    return make_tuple("", "", "");
  }

};
