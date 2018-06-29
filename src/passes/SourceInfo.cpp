#include "SourceInfo.hpp"
#include "Utility.hpp"

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>

#include <fstream>
#include <sstream>
#include <list>

using namespace std;
using namespace llvm;

cl::opt <string> InputFileName("functions", cl::value_desc("filename"),
                               cl::desc("File with list of functions"),
                               cl::init("functions.txt"));
cl::opt <string> SourceFileName("source", cl::value_desc("filename"),
                                cl::desc("Annotated file with source"),
                                cl::init("functions-annotated.txt"));
cl::opt <string> FilterFileName("filter", cl::value_desc("filename"),
                                cl::desc("Filtered list of functions"),
                                cl::init("functions-filtered.txt"));

bool SourceInfo::runOnModule(Module &M) {


  // populating function list
  ifstream inFile(InputFileName.c_str());
  string name;
  list <string> functions;

  if (!inFile) {
    errs() << "Unable to open " << InputFileName << '\n';
    exit(1);
  }

  while (inFile >> name) {
    functions.push_back(name);
  }

  error_code ec;
  source_file.reset(
      new raw_fd_ostream(SourceFileName.c_str(), ec, sys::fs::F_None));
  filter_file.reset(
      new raw_fd_ostream(FilterFileName.c_str(), ec, sys::fs::F_None));


  // printing names
  for (list<string>::iterator it = functions.begin();
       it != functions.end(); it++) {
    name = *it;

    if (Function * f = M.getFunction(name)) {

      if (!f->isDeclaration()) {
        unsigned minline = INT_MAX;
        unsigned maxline = 0;
        StringRef file;
        StringRef dir;

        for (Function::iterator b = f->begin(), be = f->end(); b != be; b++) {
          for (BasicBlock::iterator i = b->begin(), ie = b->end();
               i != ie; i++) {

            if (DILocation * loc = i->getDebugLoc()) {
              unsigned line = loc->getLine();

              if (line > maxline) {
                maxline = line;
              } else if (line < minline) {
                minline = line;
              }
              file = loc->getFilename();
            }
          }
        }

        if (maxline != 0 && minline != INT_MAX) {
          *source_file << name << " " << file << " " << minline
                       << " " << maxline << "\n";
          *filter_file << name << "\n";
        }
      }
    }
  }

  return false;
}


void SourceInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}


char SourceInfo::ID = 0;
static const RegisterPass <SourceInfo> registration("source-info",
                                                    "Printing source information for functions");
