#include "Names.hpp"
#include "BranchSafety.hpp"
#include "Utility.hpp"
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/Pass.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/InstrTypes.h>
#include "llvm/IR/InstIterator.h"
#include <sstream>

using namespace llvm;
using namespace std;
using namespace ep;

enum ErrorCodesSign { Negative, Positive };
cl::opt<ErrorCodesSign>
OptionSign(
  "sign",
  cl::desc("Error codes sign:"),
  cl::values(
    clEnumValN(Negative, "negative", "negative (default"),
    clEnumValN(Positive, "positive", "positive"),
    clEnumValEnd
    )
  );

char BranchSafetyPass::ID = 0;
static llvm::RegisterPass<BranchSafetyPass> X("branch-safety", 
  "Mark names as not containing error-codes", false, false);

bool BranchSafetyPass::runOnModule(Module &M) {
  names = &getAnalysis<NamesPass>();

  if (testing) {
    test_out_file.open(test_out_path, std::ofstream::out);    
  }

  // Infer the error codes sign by looking at the first code in the file
  map<int, vn_t> error_names = names->getErrorNames();
  const auto first_ec = error_names.begin();
  if (first_ec->first < 0) {
    OptionSign = ErrorCodesSign::Negative;
  } else if (first_ec->first > 0) {
    OptionSign = ErrorCodesSign::Positive;
  } else {
    errs() << "Using 0 as an error code is not supported.\n";
    abort();
  }
  
  for (auto f = M.begin(), e = M.end(); f != e; ++f) {
    for (inst_iterator i = inst_begin(&*f), ie = inst_end(&*f); i != ie; ++i) {
      if (BranchInst *bi = dyn_cast<BranchInst>(&*i)) {
        visitBranchInst(*bi);
      }
    }
  }

  if (testing) {
    test_out_file.close();
  }

  return false;
}

set<VarName> BranchSafetyPass::getSafeNames(BasicBlock &BB) {
  set<VarName> safe_names;

  set<Value*> safe_for_bb = safe_values[&BB];
  for (set<Value*>::iterator i = safe_for_bb.begin(), e = safe_for_bb.end(); i != e; ++i) {
    Value *lookup = *i;
    vn_t name = names->getVarName(lookup);
    if (name) {
      safe_names.insert(*name);
    }
  }

  return safe_names;
}

pair<BasicBlock*, BasicBlock*> BranchSafetyPass::getBranchBlocks(BranchInst *branch) {
  if (!branch) return make_pair(nullptr, nullptr);

  if (branch_pairs.find(branch) == branch_pairs.end()) {
    return make_pair(nullptr, nullptr);
  }

  std::pair<TriVal, TriVal> branch_pair = branch_pairs.at(branch);

  BasicBlock *unsafe_block = nullptr;
  BasicBlock *safe_block   = nullptr;
  switch (branch_pair.second) {
  case TriVal::T:
    unsafe_block = branch->getSuccessor(0);
    safe_block   = branch->getSuccessor(1);
    break;
  case TriVal::F:
    unsafe_block = branch->getSuccessor(1);
    safe_block   = branch->getSuccessor(0);
    break;
  case TriVal::N:
    break;
  }

  return make_pair(safe_block, unsafe_block);
}

bool BranchSafetyPass::tested_at(VarName var, ICmpInst *icmp) {
  if (icmps.find(icmp) == icmps.end()) return false;

  for (VarName vn : icmps[icmp]) {
    if (vn == var) {
      return true;
    }
  }

  return false;
}

// Converts IS_ERR(x) to x
// This allows us to prune branches based on IS_ERR tests
// without interprocedural analysis at this stage
Value* BranchSafetyPass::strip_is_err(Value* operand) {
  if (CallInst* call = dyn_cast<CallInst>(operand)) {
    Function *f = call->getCalledFunction();
    if (f) {
      string fn = f->getName();

      // FIXME: This is only OK for Linux, so it should not be hardcoded.
      // We test to see if the function starts with this name
      // because LLVM duplicates IS_ERR into *many* different definitions
      // IS_ERR, ... IS_ERR2343, IS_ERR47, ...
      // Preventing this duplication would be better, but I don't know how to do it.
      if (fn.find("IS_ERR") == 0 ||
          fn.find("IS_ERR_VALUE") == 0 ||
          fn.find("IS_ERR_OR_NULL") == 0) {
        return call->getArgOperand(0);
      }
    }
  }

  return operand;
}

set<VarName> BranchSafetyPass::codesThroughPred(llvm::ICmpInst *icmp) {
  set<VarName> ret;

  if (explicit_equality.find(icmp) != explicit_equality.end()) {
    ret.insert(explicit_equality.at(icmp));
  }

  return ret;
}

void BranchSafetyPass::visitBranchInst(BranchInst &I) {
  if (I.isUnconditional()) {
    return;
  }

  Value *condition = I.getCondition();
  ICmpInst *cmp = dyn_cast<ICmpInst>(condition);
  if (cmp == nullptr) {
    return;
  }

  map<int, vn_t> error_names = names->getErrorNames();
  bool right_constant = false;    // (err < 0) vs. (0 < err)
  ConstantInt *num  = nullptr;    // The constant being var is being compared to

  if ((num = dyn_cast<ConstantInt>(cmp->getOperand(1)))) {
    right_constant = true;
  } else {
    num = dyn_cast<ConstantInt>(cmp->getOperand(0));
  }

  bool is_zero = num && num->isZero();
  bool is_ec   = num && (error_names.find(num->getSExtValue()) != error_names.end());

  vn_t ec_name = nullptr;
  if (is_ec) {
    ec_name = error_names.at(num->getSExtValue());
  }

  // Not an error test, as far as we can tell
  if (!is_zero && !is_ec) {
    if (testing) {
      test_out_file << getSource(cmp) << " (N,N)\n";
    }
    return;
  }

  // At this point the icmp matches pattern that is potentially testing EC
  vn_t pred_var;
  if (right_constant) {
    pred_var = names->getVarName(cmp->getOperand(0));
  } else {
    pred_var = names->getVarName(cmp->getOperand(1));
  }
  if (pred_var) {
    icmps[cmp].insert(*pred_var);
  }

  // Which branch is the value safe in? true, false, or neither
  Branch safe_branch, eh_branch;

  bool positive_switch = false;
  switch (cmp->getPredicate()) {
    case CmpInst::Predicate::ICMP_SLE:
    case CmpInst::Predicate::ICMP_SLT:
      safe_branch.direction = right_constant ? TriVal::F : TriVal::T;
      eh_branch = safe_branch;
      eh_branch.flip();
      positive_switch = true;
      break;
    case CmpInst::Predicate::ICMP_SGT:
    case CmpInst::Predicate::ICMP_SGE:
      safe_branch.direction = right_constant ? TriVal::T : TriVal::F;
      eh_branch = safe_branch;
      eh_branch.flip();
      positive_switch = true;
      break;
    case CmpInst::Predicate::ICMP_EQ:
      if (is_zero) {
        safe_branch.direction = TriVal::T;
        eh_branch.direction   = TriVal::F;
      } else if (is_ec) {
	explicit_equality[cmp] = *ec_name;
        safe_branch.direction = TriVal::N;
        eh_branch.direction   = TriVal::T;
      }
      break;
    case CmpInst::Predicate::ICMP_NE:
      if (is_zero) {
        safe_branch.direction = TriVal::F;
        eh_branch.direction   = TriVal::T;
      } else if (is_ec) {
	explicit_inequality[cmp] = *ec_name;
        safe_branch.direction = TriVal::N;
        eh_branch.direction   = TriVal::N;
      }
      break;
    default:
      safe_branch.direction = TriVal::N;
      break;
  }

  if (OptionSign == ErrorCodesSign::Positive && positive_switch) {
    safe_branch.flip();
    eh_branch.flip();
  }

  if (testing) {
    test_out_file << getSource(cmp) << " (" << safe_branch.str() << "," << eh_branch.str() << ")\n";
  }

  // Set the branch pair that will be returned by getBranchPair()
  branch_pairs[&I] = make_pair(safe_branch.direction, eh_branch.direction);

  // Declare variable safe in appropriate block
  // notsafe_bb is *not* guaranteed to be an error-handling block,
  // but error_i is the first error-handling instruction if not nullptr
  BasicBlock *safe_bb     = nullptr;
  BasicBlock *notsafe_bb  = nullptr;
  if (safe_branch.direction == TriVal::T) {
    safe_bb = I.getSuccessor(0);
    notsafe_bb = I.getSuccessor(1);
  } else if (safe_branch.direction == TriVal::F) {
    safe_bb = I.getSuccessor(1);
    notsafe_bb = I.getSuccessor(0);
  }

  if (safe_bb) {
    // TODO: Change to post-dominator (high priority)
    // Make sure this isn't preceded by not_safe block
    // Which would make both branches safe
    for (pred_iterator PI = pred_begin(safe_bb), E = pred_end(safe_bb); PI != E; ++PI) {
      BasicBlock *Pred = *PI;
      if (Pred == notsafe_bb) {
        return;
      }
    }

    if (right_constant) {
      Value* safe_value = strip_is_err(cmp->getOperand(0));
      safe_values[safe_bb].insert(safe_value);
    } else {
      Value* safe_value = strip_is_err(cmp->getOperand(1));
      safe_values[safe_bb].insert(safe_value);
    }
  }
}

void BranchSafetyPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<NamesPass>();
}

