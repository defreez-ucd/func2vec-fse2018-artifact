#include "ControlFlow.hpp"
#include "BranchSafety.hpp"
#include "Traces.hpp"
#include "HandlersPass.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include <unistd.h>

using namespace llvm;
using namespace std;

void usage() {
  cerr << "Usage: " << "tracegen -e <codes file> -b <bitcode file> [-d dbfile] [-i handlers file]\n";
  cerr << "-d to write results to sqlite database\n";
}

int main(int argc, char **argv) {
  string bitcode_path, ec_path, db_path, handlers_path;
  bool ec_context       = false;

  int c;

  while ((c = getopt(argc, argv, "e:c:b:d:p:i:")) != EOF) {
    switch (c) {
    case 'e':
      ec_path = optarg;
      break;
    case 'b':
      bitcode_path = optarg;
      break;
    case 'd':
      db_path = optarg;
      break;
    case 'i':
      handlers_path = optarg;
      break;
    case ':':
    case '?':
      usage();
      return 1;
    }
  }

  if (bitcode_path.empty()) {
    usage();
    return 1;
  }
  if (ec_path.empty()) {
    usage();
    return 1;
  }
  if (handlers_path.empty()) {
    usage();
    return 1;
  }

  SMDiagnostic Err;
  std::unique_ptr<Module> Mod(parseIRFile(bitcode_path, Err, getGlobalContext()));

  if (!Mod) {
    Err.print(argv[0], errs());
    return 1;
  }

  legacy::PassManager PM;
  NamesPass *names = new NamesPass(ec_path);
  PM.add(names);

  BranchSafetyPass *safety = new BranchSafetyPass();
  PM.add(safety);

  ControlFlowPass *cfp = new ControlFlowPass();
  PM.add(cfp);

  PostDominatorTree *postdom = new PostDominatorTree();
  PM.add(postdom);

  MemoryDependenceAnalysis *mda = new MemoryDependenceAnalysis();
  PM.add(mda);

  cerr << "Running frontend passes...\n";
  PM.run(*Mod);

  Traces traces(cfp, db_path, safety, names, postdom);
  traces.read_handlers(handlers_path);
  traces.initialize();

  cerr << "Writing traces...\n";
  traces.generate(cout);

  return 0;
}
