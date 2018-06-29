#include "error-codes.h"
#include "llvm/Pass.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CFG.h"
#include <iostream>

using namespace llvm;
using namespace std;

// This pass identifies all instructions that use an error code, and then
// prints the source location of preceding terminator instructions.

// This pass only works at all if the error codes are rewritten to strange numbers.
// Common integers appear all over the place. For e.g. 32 is the the number of bits in i32

struct ErrorCodeInstructions : public ModulePass {
  static char ID;
  ErrorCodeInstructions() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    for (auto f = M.begin(), e = M.end(); f != e; ++f) {
      Function *function = &*f;
      runOnFunction(*function);
    }
    return false;
  }

  void runOnFunction(Function &F) {
    for (inst_iterator i = inst_begin(F), ie = inst_end(F); i != ie; ++i) {
      Instruction *inst = &*i;
      assert(inst);

      // Go through the operands
      // If the operand is a constant int then print the instruction location

      // CURRENT: Loop through all of the operands and print source location if
      // a constant int is used.
      for (unsigned op = 0, last = inst->getNumOperands(); op != last; ++op) {
        // If this operand is a constant integer
        if (ConstantInt *CI = dyn_cast<ConstantInt>(inst->getOperand(op))) {
          if (ERROR_CODES.find(CI->getSExtValue()) == ERROR_CODES.end()) {
            continue;
          }

          // At this point inst uses an error code as an operand.
          BasicBlock *inst_bb = inst->getParent();
          for (auto it = pred_begin(inst_bb), et = pred_end(inst_bb); it != et; ++it) {
            BasicBlock *predecessor = *it;
            Instruction *terminator = predecessor->getTerminator();
            printLocation(*terminator);
          }
        }
      }
    }
  }

  void printLocation(Instruction &I) {
    if (DILocation *loc = I.getDebugLoc()) {
      unsigned line = loc->getLine();
      string file = loc->getFilename();
      cout << file << ":" << line << endl;
    }
  }

  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
  }
};

char ErrorCodeInstructions::ID = 0;
static const RegisterPass<ErrorCodeInstructions> registration("errorcode-instructions", "Print source location of all instructions that use error codes.");
