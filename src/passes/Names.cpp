#include "Names.hpp"
#include "llvm/IR/BasicBlock.h"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/CFG.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <unordered_set>

using namespace llvm;
using namespace std;

char NamesPass::ID = 0;
static llvm::RegisterPass<NamesPass> N("names", "Propagate Names", false, false);

cl::opt<string> ECFileName("error-codes", cl::value_desc("ecfilename"), cl::desc("Error Codes file"));

bool NamesPass::runOnModule(Module &M) {
  module = &M;

  // Populate errorNames map
  string ecpath;
  if (!ecpath_arg.empty()) {
    ecpath = ecpath_arg;
  } else {
    ecpath = ECFileName;
  }

  if (!ecpath.empty()) {
    // This is a hack to allow an empty error codes list because people don't like having to pass
    // the error codes file as an argument. Every error code will get the name OK.
    ifstream inFile(ecpath);
    if (!inFile) {
      errs() << "NamesPass: unable to open error-codes file " << ECFileName
             << "\n";
      exit(1);
    }

    string name;
    unsigned int value;
    map<unsigned int, string> en;
    while (inFile >> name >> value) {
      error_names[value] = make_shared<ErrorName>("TENTATIVE_" + name);
    }
    inFile.close();
  }

  // Collect global variables
  for (Module::global_iterator i = M.global_begin(), e = M.global_end();
       i != e; ++i) {
    VarType t = resolveType(&*i);
    if (ConstantStruct *literal = getStruct(&*i)) {

      // Create struct abstraction based on type
      // TODO: Merge into utility function with visitStore loop
      StructType *st = literal->getType();
      unsigned num_elems = st->getNumElements();
      for (unsigned idx = 0; idx < num_elems; ++idx) {
        Constant *element = literal->getAggregateElement(idx)->stripPointerCasts();

        if (Function *f = dyn_cast<Function>(element)) {
          string approx_str = getApproxName(st);
          if (!approx_str.empty()) {
            MemoryName approx_name(approx_str, 0, idx, VarScope::GLOBAL, nullptr);
            backFunction(approx_name, f);
          }
          if (!i->getName().empty()) {
            MemoryName actual_name(i->getName(), 0, idx, VarScope::GLOBAL, nullptr);
            backFunction(actual_name, f);
          }
        }
      } // Each element
    }

    if (t != VarType::EMPTY) {
      string name = i->getName();
      names[&*i] = make_shared<VarName>(name, VarScope::GLOBAL, t);
    }
  }

  // Prepend function name to function arguments,
  // populate raw function names for function pointers
  // and add global return names
  for (Module::iterator function = M.begin(), e = M.end(); function != e; ++function) {
    if (function->isIntrinsic() || function->isDeclaration()) {
      continue;
    }
    setupFunction(&*function);
  }
  for (Module::iterator function = M.begin(), e = M.end(); function != e; ++function) {
    if (function->isIntrinsic() || function->isDeclaration()) {
      continue;
    }
    visit(&*function);
  }

  return false;
}

// TODO: typedef
map<string, set<string>> NamesPass::get_bootstrap_functions() {
  map<string, set<string>> ret;

  for (map<VarName, vn_t>::iterator i = memory_model.begin(), e = memory_model.end(); i != e; ++i) {
    string idx = i->first.name();
    vn_t value = i->second;

    // TODO: Isolate string parsing somewhere
    if (idx.find("#") != string::npos) continue;
    if (ret.find(idx) == ret.end()) {
      ret[idx] = set<string>();
    }

    if (value->type == VarType::FUNCTION) {
      ret[idx].insert(value->name());
    } else if (value->type == VarType::MULTI) {
      mul_t mul_value = static_pointer_cast<MultiName>(value);
      for (const vn_t v : mul_value->names()) {
        if (v->type == VarType::FUNCTION) {
          ret[idx].insert(v->name());
        }
      }
    }
  }

  for (map<string, std::set<string>>::iterator i = ret.begin(), e = ret.end(); i != e; ++i) {
    if (i->second.size() < 2) {
      ret.erase(i);
    }
  }

  return ret;
}

map<int, vn_t> NamesPass::getErrorNames() {
  return error_names;
}

std::string NamesPass::getApproxName(StructType *st) {
  string approx_str = st->getName();

  string::size_type struct_dot = approx_str.find("struct.");
  if (struct_dot == string::npos) {
    struct_dot = 0;
  } else {
    struct_dot = 7;
  }
  string::size_type dot = approx_str.find('.', struct_dot + 1);
  if (dot != string::npos) {
    approx_str = approx_str.substr(0, dot);
  }

  return approx_str;
}

void NamesPass::setupFunction(Function *function) {
  vn_t extant = getVarName(function);
  if (!extant) {
    fn_t fname = make_shared<FunctionName>(function->getName());
    fname->function = &*function;
    names[&*function] = fname;
    extant = fname;
  }

  fn_t fname = static_pointer_cast<FunctionName>(extant);

  Function::ArgumentListType &args = function->getArgumentList();
  for (auto a = args.begin(), ae = args.end(); a != ae; ++a) {
    Value *arg = &*a;
    VarType t = resolveType(arg);
    if (t == VarType::EMPTY) {
      continue;
    }

    // This is were the formal arg names are generated
    // The actual names are generate in visitStoreInst
    string an = fname->name() + "$" + arg->getName().str();
    names[&*a] = make_shared<VarName>(an, VarScope::GLOBAL, t);
  }
}

// Two requests for VarName of same instruction will yield same vn_t.
// What about same name for different instructions?
vn_t NamesPass::getVarName(const Value *V) {
  if (!V) {
    errs() << "FATAL ERROR: VarName requested for null value.\n";
    abort();
  }

  const Value *stripped_V;
  if (!isa<GetElementPtrInst>(V)) {
    stripped_V = V->stripPointerCasts();
  } else {
    stripped_V = V;
  }

  // Error code
  if (const ConstantInt *c = dyn_cast<ConstantInt>(stripped_V)) {
    int numberValue = c->getLimitedValue();
    if (error_names.find(numberValue) != error_names.end()) {
      return error_names.at(numberValue);
    } else {
      return EC_OK;
    }
  }

  // Something our analysis couldn't handle
  if (names.find(stripped_V) == names.end()) {
    return nullptr;
  }

  return names.at(stripped_V);
}

string NamesPass::getStackName(Instruction &I) {
  string fname = I.getParent()->getParent()->getName();
  string iid;

  if (stack_iids.find(&I) != stack_iids.end()) {
    iid = stack_iids[&I];
  } else {
    iid = to_string(stack_cnt++);
    stack_iids[&I] = iid;
  }

  return fname + "." + iid;
}

string NamesPass::getDummyName(Instruction *I) {
  string fname = I->getParent()->getParent()->getName();
  return fname + "." + to_string(dummy_cnt++) + "x";
}

// Basic blocks are named by the iid of their first instruction
// bbe is appended for the entry point and bbx for the exit
pair<string, string> NamesPass::getBBNames(BasicBlock &BB) {
  pair<string, string> names;

  Instruction &front = BB.front();
  string bb_enter = getStackName(front) + "bbe";
  string bb_exit = getStackName(front) + "bbx";

  names = make_pair(bb_enter, bb_exit);
  return names;
}

// Call names are just plain strings (stack locations)
string NamesPass::getCallName(Function &F) {
  string fname = F.getName();
  return fname + ".0";
}

// Get the names of the functions that are called by this instruction
// This is not the same as the callname stack location
// TODO: Disambiguate this
vector<string> NamesPass::getCalleeNames(const CallInst &CI) {
  vector<string> callee_names;

  const Value *cv = CI.getCalledValue();
  if (!cv) {
    return callee_names;
  }

  const Function *f = CI.getCalledFunction();
  if (f && !f->isIntrinsic()) {
    callee_names.push_back(f->getName());
    return callee_names;
  }

  vn_t callee_name = getVarName(cv);
  if (callee_name) {
    mul_t callees;
    if (callee_name->type == VarType::MULTI) {
      callees = static_pointer_cast<MultiName>(callee_name);
    } else {
      callees = make_shared<MultiName>();
      callees->insert(callee_name);
    }

    for (vn_t callee_vn : callees->names()) {
      if (callee_vn->type != VarType::FUNCTION) {
        continue;
      }
      fn_t callee_fn = static_pointer_cast<FunctionName>(callee_vn);
      callee_names.push_back(callee_fn->name());
    }
  }

  return callee_names;
}

set<Value *> NamesPass::getLocalValues(Function &F) {
  return locals[&F];
}

VarType NamesPass::resolveType(Value *V) {
  Type *t = V->getType();

  if (const GlobalVariable *gv = dyn_cast<GlobalVariable>(V)) {
    if (gv->hasUnnamedAddr()) {
      return VarType::EMPTY;
    }
  }

  if (isa<ConstantPointerNull>(V)) {
    return VarType::EMPTY;
  }

  // We assume any pointer could point to an int
  if (t->isPointerTy() || t->isIntegerTy(32) || t->isIntegerTy(64)) {
    return VarType::INT;
  }

  return VarType::EMPTY;
}

bool NamesPass::filter(Type *T) {
  if (T->isPointerTy()) {
    return true;
  }

  return T->isIntegerTy(32) || T->isIntegerTy(64);
}

ConstantStruct *NamesPass::getStruct(Value *V) {
  Type *t = V->getType();
  Constant *c = dyn_cast<Constant>(V);
  unsigned num_operands = 0;
  if (c) {
    // Filter out things like null which are constant but with no operands
    num_operands = c->getNumOperands();
  }

  // Struct constants are pointers
  if (isa<PointerType>(t) && num_operands >= 1) {
    if (ConstantStruct *literal = dyn_cast<ConstantStruct>(c->getOperand(0))) {
      return literal;
    }
  }

  return nullptr;
}

void NamesPass::backFunction(MemoryName index, Function *f) {
  fn_t fpoint = make_shared<FunctionName>(f->getName());
  fpoint->function = f;
  mul_t fp_container = make_shared<MultiName>();
  fp_container->insert(fpoint);
  updateMemory(index, fp_container);
}

void NamesPass::updateMemory(MemoryName index, vn_t update) {
  if (index.name() == "union" || index.name() == "union.0.0") {
    return;
  }

  vn_t backing_name = memory_model[index];

  mul_t update_mul = nullptr;
  mul_t backing_mul = nullptr;
  if (backing_name && backing_name->type == VarType::MULTI) {
    backing_mul = static_pointer_cast<MultiName>(backing_name);
  }
  if (update->type == VarType::MULTI) {
    update_mul = static_pointer_cast<MultiName>(update);
  }

  if (update_mul && backing_mul) {
    // Both MultiName
    for (vn_t vn : update_mul->names()) {
      backing_mul->insert(vn);
    }
  } else if (!update_mul && backing_mul) {
    // Only receiver is MultiName
    backing_mul->insert(update);
  } else if (update_mul && !backing_mul && backing_name) {
    update_mul->insert(backing_name);
    memory_model[index] = update_mul;
  } else if (update->type == VarType::FUNCTION && backing_name && update->name() != backing_name->name()) {
    // Where MultiNames are created, only handle functions for now
    // Removing the VarType::FUNCTION filter above will require adjustiing
    // several visitor functions to handle MultiNames
    mul_t multi = make_shared<MultiName>();
    multi->insert(backing_name);
    multi->insert(update);
    memory_model[index] = multi;

  } else {
    memory_model[index] = update;
  }
}

mem_t NamesPass::getLoadIndex(const Value *v) const {
  if (load_index.find(v) == load_index.end()) {
    return nullptr;
  }
  return load_index.at(v);
}

// Name of load instruction is the name of what it loads
void NamesPass::visitLoadInst(LoadInst &I) {
  Value *from = I.getOperand(0);

  if (ConstantExpr *expr = dyn_cast<ConstantExpr>(from)) {
    // To avoid duplicating logic for expressions,
    // visit this as if it were an instruction
    // (generates GEP name)
    Instruction *i_expr = expr->getAsInstruction();
    visit(i_expr);
    from = i_expr;
  }

  if (isa<GetElementPtrInst>(from)) {
    vn_t gep_name = getVarName(from);
    if (!gep_name || gep_name->type != VarType::MEMORY) {
      return;
    }

    mem_t gep_mem = static_pointer_cast<MemoryName>(gep_name);

    // Need to look up backing VarName
    if (memory_model.find(*gep_mem) != memory_model.end()) {
      // We already have the backing name
      names[&I] = memory_model.at(*gep_mem);
      load_index[&I] = gep_mem;
      vn_t vn = names[&I];
    } else if (GlobalValue *gv = module->getNamedValue(gep_mem->base_name)) {
      // Global constant GEP lookups
      if (ConstantStruct *literal = getStruct(gv)) {
        Constant *element = literal->getAggregateElement(gep_mem->idx2);
        if (element) {
          element = element->stripPointerCasts();
          vn_t el_name = getVarName(element);
          names[&I] = el_name;
        }
        load_index[&I] = gep_mem;
      } // constant struct
    } else {
      // Try approximating by struct type of load target
      GetElementPtrInst *gep_inst = dyn_cast<GetElementPtrInst>(from);
      mem_t index = getApproxName(*gep_inst);

      if (index && memory_model.find(*index) != memory_model.end()) {
        names[&I] = memory_model.at(*index);
        load_index[&I] = index;
      }
    }
  } else {
    // Non-GEP
    // We already have a usable name for load, just copy the reference
    vn_t ref = getVarName(from);

    // TODO: If this is VarType::MEMORY log imprecision

    if (ref && ref->type != VarType::MEMORY) {
      names[&I] = ref;
    }
  }
}

mem_t NamesPass::getApproxName(GetElementPtrInst &I) {
  mem_t ret = nullptr;

  Type *t = I.getOperand(0)->getType()->getContainedType(0);
  if (StructType *st = dyn_cast<StructType>(t)) {
    Value *idx2 = I.getOperand(2);
    ConstantInt *idx2_int = dyn_cast<ConstantInt>(idx2);
    string approx_str = getApproxName(st);
    ret = make_shared<MemoryName>(approx_str, 0, idx2_int->getLimitedValue(), VarScope::GLOBAL, nullptr);
  }

  return ret;
}

// Local variables have function name prepended
void NamesPass::visitAllocaInst(AllocaInst &I) {
  vn_t vn = names[&I];

  // Already have a name from llvm.dbg
  if (vn) {
    return;
  }

  // No real var name, generate an intermediate name (cabs2cil_)
  string intermediate = generateIntermediateName();

  Function *f = I.getParent()->getParent();
  names[&I] = make_shared<IntName>(intermediate, f);
  locals[f].insert(&I);
}

string NamesPass::generateIntermediateName() {
  return "cabs2cil_" + to_string(intermediate_cnt++);
}

// Set name to name of global exchange var
void NamesPass::visitCallInst(CallInst &I) {

  // dbg.declares give us a more reliable name than getName
  // getVarName will prefer information provided here
  if (DbgDeclareInst *dbg = dyn_cast<DbgDeclareInst>(&I)) {
    DIVariable *DV = static_cast<DIVariable *>(dbg->getVariable());
    Value *V = dbg->getAddress();

    Function *f = I.getParent()->getParent();
    string fname = f->getName();
    string vname = DV->getName();

    names[V] = make_shared<IntName>(fname + "#" + vname, f);
    locals[f].insert(V);
    return;
  }

  if (isa<IntrinsicInst>(&I)) {
    return;
  }

  Value *cv = I.getCalledValue();
  if (!cv) {
    return;
  }

  vn_t callee = getVarName(cv);
  if (!callee) {
    return;
  }

  if (callee->type == VarType::FUNCTION) {
    // Indirect call with a single value
    fn_t function_name = static_pointer_cast<FunctionName>(callee);
    string callee_name = function_name->function->getName();
    vn_t exchange = make_shared<VarName>(callee_name + "$return", VarScope::GLOBAL, VarType::INT);
    names[&I] = exchange;
  } else if (callee->type == VarType::MULTI) {
    // Indirect call with multiple possibilities
    mul_t multi_callee = static_pointer_cast<MultiName>(callee);
    mul_t exchange_container = make_shared<MultiName>();

    for (vn_t fn : multi_callee->names()) {
      if (fn->type != VarType::FUNCTION) {
        continue;
      }

      fn_t function_name = static_pointer_cast<FunctionName>(fn);
      string callee_name = function_name->function->getName();
      vn_t exchange = make_shared<VarName>(callee_name + "$return",
                                           VarScope::GLOBAL, VarType::INT);
      exchange_container->insert(exchange);
    }
    names[&I] = exchange_container;

  } else {
    // Normal call
    Function *f = I.getCalledFunction();
    if (f) {
      string callee_name = f->getName();
      vn_t exchange = make_shared<VarName>(callee_name + "$return", VarScope::GLOBAL, VarType::INT);
      names[&I] = exchange;
    }
  }
}

void NamesPass::visitStoreInst(StoreInst &I) {
  Value *sender = I.getOperand(0)->stripPointerCasts();
  Value *receiver = I.getOperand(1);

  vn_t sender_name = getVarName(sender);
  vn_t receiver_name = getVarName(receiver);
  if (!sender_name || !receiver_name) {
    return;
  }

  // We handle storing to GEP separately;
  // It is indexed by MemoryNames (GEP indexes), pointing at "backing" VarNames 
  if (isa<GetElementPtrInst>(receiver)) {
    // Check to see if memory model already has a VarName for GEP
    vn_t gep_name = getVarName(receiver);
    if (!gep_name) {
      return;
    }
    if (gep_name->type != VarType::MEMORY) {
      errs() << "FATAL ERROR: GEP with non-memory name\n";
      abort();
    }
    mem_t mem_name = static_pointer_cast<MemoryName>(gep_name);

    updateMemory(*mem_name, sender_name);
    return;
  }

  // Handle global struct literals with function pointers in them
  // If sender is a struct literal then we need to initialize backing VarNames for receiver
  Type *sender_type = sender->getType();
  if (ConstantStruct *literal = getStruct(sender)) {
    // Go through each element in struct literal and create backing VarName
    StructType *st = cast<StructType>(sender_type->getContainedType(0));
    unsigned num_elems = st->getNumElements();
    for (unsigned idx = 0; idx < num_elems; ++idx) {
      Constant *element = literal->getAggregateElement(idx)->stripPointerCasts();

      if (Function *f = dyn_cast<Function>(element)) {
        if (receiver_name && receiver_name->type != VarType::MULTI) {
          string r_name = receiver_name->name();
          MemoryName mem_name(r_name, 0, idx, receiver_name->scope, receiver_name->parent);
          backFunction(mem_name, f);
        }
      }
    } // Each element
  }

  // To transparently merge pointer names
  receiver = receiver->stripPointerCasts();

  VarType t = resolveType(sender);
  if (t == VarType::EMPTY) {
    return;
  }

  // If sender is a function, then this is a function pointer
  // Copy sender name to receiver name. This will overwrite the
  // receiver name, which is OK in this case.
  if (sender_name->getType() == VarType::FUNCTION) {
    names[receiver] = sender_name;
  }

  Function *f = I.getParent()->getParent();
  // Check to see if this is a copy from formal to actual arguments
  Function::ArgumentListType &args = f->getArgumentList();
  for (auto a = args.begin(), ae = args.end(); a != ae; ++a) {
    if (&*a == sender) {
      string fname = f->getName();
      string arg_name = a->getName();
      string receiver_name = fname + "#" + arg_name;
      names[receiver] = make_shared<IntName>(receiver_name, f);
      locals[f].insert(receiver);
    }
  }
}

// add, sub, etc.
// Any binary operations imply it wasn't an error code
void NamesPass::visitBinaryOperator(BinaryOperator &I) {
  names[&I] = make_shared<ErrorName>("OK");
}

void NamesPass::visitPtrToIntInst(PtrToIntInst &I) {
  names[&I] = getVarName(I.getOperand(0));
}

void NamesPass::visitIntToPtrInst(IntToPtrInst &I) {
  names[&I] = getVarName(I.getOperand(0));
}

// We take phi instructions and create a multiname from all possible values
void NamesPass::visitPHINode(PHINode &I) {
  mul_t phi = make_shared<MultiName>();
  for (unsigned i = 0, e = I.getNumIncomingValues(); i != e; ++i) {
    vn_t vn = getVarName(I.getIncomingValue(i));
    if (!vn) continue;
    if (vn->getType() == VarType::MULTI) {
      mul_t mn = static_pointer_cast<MultiName>(vn);
      for (vn_t sub : mn->names()) {
        phi->insert(sub);
      }
    } else {
      phi->insert(vn);
    }
  }
  names[&I] = phi;
}

void NamesPass::visitSelectInst(SelectInst &I) {
  vn_t true_vn = getVarName(I.getTrueValue());
  vn_t false_vn = getVarName(I.getFalseValue());

  mul_t select_vn = make_shared<MultiName>();

  if (true_vn) {
    select_vn->insert(true_vn);
  }
  if (false_vn) {
    select_vn->insert(false_vn);
  }

  if (select_vn->names().size() > 0) {
    names[&I] = select_vn;
  }
}

void NamesPass::visitGetElementPtrInst(GetElementPtrInst &I) {
  if (I.getNumOperands() < 3) {
    return;
  }

  Value *idx1 = I.getOperand(1);
  Value *idx2 = I.getOperand(2);
  ConstantInt *idx1_int = dyn_cast<ConstantInt>(idx1);
  ConstantInt *idx2_int = dyn_cast<ConstantInt>(idx2);
  vn_t struct_name = getVarName(I.getOperand(0));

  // We only support constant indexes
  if (!idx1_int || !idx2_int) {
    return;
  }

  if (struct_name && struct_name->type != VarType::MULTI &&
      struct_name->type != VarType::EC) {
    // Give memory name based on GEP index
    // e.g. main#foo.0.0 for first element in struct foo
    vn_t mn = make_shared<MemoryName>(struct_name->name(),
                                      idx1_int->getLimitedValue(),
                                      idx2_int->getLimitedValue(),
                                      struct_name->scope, struct_name->parent);
    mn->value = &I;

    names[&I] = mn;

    if (struct_name->parent != nullptr) {
      locals[struct_name->parent].insert(&I);

    }
  } else if (!struct_name ||
             (struct_name && struct_name->type == VarType::EC)) {
    // If we don't have a name, but this is a struct, use the approximate name
    Type *t = I.getOperand(0)->getType()->getContainedType(0);
    if (StructType *st = dyn_cast<StructType>(t)) {
      string approx_str = getApproxName(st);
      uint64_t idx1_num = idx1_int->getLimitedValue();
      uint64_t idx2_num = idx2_int->getLimitedValue();
      mem_t mn = make_shared<MemoryName>(approx_str, idx1_num, idx2_num, VarScope::GLOBAL, nullptr);
      mn->value = &I;
      names[&I] = mn;
    }
  }
}

void NamesPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}
