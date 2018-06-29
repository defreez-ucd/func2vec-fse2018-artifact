#ifndef SOURCE_INFO_GUARD
#define SOURCE_INFO_GUARD 1

#include <llvm/Pass.h>

using namespace std;
using namespace llvm;

namespace llvm {
  class raw_fd_ostream;
}


class SourceInfo : public ModulePass {
  
public:
  SourceInfo() : ModulePass(ID) {}
  
  virtual bool runOnModule(Module &M);
  
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  static char ID; // Pass identification, replacement for typeid

#if __cplusplus > 199711L
  unique_ptr<raw_fd_ostream> source_file;
  unique_ptr<raw_fd_ostream> filter_file;
#else
  auto_ptr<raw_fd_ostream> source_file;
  auto_ptr<raw_fd_ostream> filter_file;
#endif


};

#endif // SOURCE_INFO_GUARD
