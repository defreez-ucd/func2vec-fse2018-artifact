#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>

using namespace std;
using namespace llvm;


struct FlatFunctions : public FunctionPass {
  static char ID;
  FlatFunctions() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    bool output = false;

    for (inst_iterator i = inst_begin(F), ie = inst_end(F); i != ie; ++i) {
      Instruction *inst = &*i;
      if (CallInst *call = dyn_cast<CallInst>(inst)) {
        Function *f = call->getCalledFunction();
        if (f) {
          if (f->isIntrinsic()) {
            continue;
          }
          string call_name = f->getName().str();
          auto idx = call_name.find('.');
          if (idx != string::npos) {
            call_name = call_name.substr(0, idx);
          }
          cout << call_name << " ";
          output = true;
        }
      }
    }

    if (output) {
      cout << endl;
    }
  }
};

char FlatFunctions::ID = 0;
static const RegisterPass<FlatFunctions> registration("flat-functions", "Flat list of function sentences.");
