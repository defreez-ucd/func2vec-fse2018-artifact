// This pass is responsible for keeping a map of Values to names
// These names are what we want to use in the WPDS file
// getVarName is the function most commonly used by clients

// Local Variables
// ===============
// Local variables have the following forms:
// foo#x: Local variable x in function foo.
//        These are generated from llvm.dbg.declare calls.
//
// There is also a map calle locals indexed by pointers to function values
// Each element is the set of names local to that function
// These are drawn from the function arguments and alloca instructions

// Global variables
// ================
// There are three ways that a global variable can come about
// 1. Global variables that actually exist in the program.
//    These are collected in NamesPass::runOnModule
// 2. Variables named foo$return to hold the return value from function foo
//    These are generated when addGlobalReturn is called
//    The names map also holds a mapping from the CallInst value to this name
// 3. Variables named foo$arg which hold the value of arg passed to function foo
//    These are collect in NamesPass::runOnModule

// Function Pointers
// =================
// Function pointers are handled *if* the pointer is stored in a memory location
// that we handle (pretty much just structs at the moment). 
// Function pointers stored in any other variable will probably only get a single
// possible function name, and function pointers as arguments are not handled at all.

#ifndef NAMESPASS_H
#define NAMESPASS_H

#include "llvm/Pass.h"
#include "VarName.hpp"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstVisitor.h"
#include <set>
#include <unordered_set>
#include <queue>

using namespace std;

class NamesPass : public llvm::ModulePass, public llvm::InstVisitor<NamesPass> {
  // Functions are virtual to allow mocking

public:
  static char ID;
  NamesPass() : llvm::ModulePass(ID) {}
  NamesPass(string ecfile) : llvm::ModulePass(ID), ecpath_arg(ecfile) {}

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  // Get a pointer to the VarName associated with an LLVM Value
  // Setting allow_ec to false will return nullptr for error codes
  virtual vn_t getVarName(const llvm::Value *V);

  // Get the stack name to use for an instruction.
  string getStackName(llvm::Instruction &I);

  // Get the name to use for a basic block
  pair<string,string> getBBNames(llvm::BasicBlock &BB);

  // Get the name to use for a call to a function
  string getCallName(llvm::Function &F);

  // Get the names of all of the functions called
  std::vector<string> getCalleeNames(const llvm::CallInst &CI);

  // Get the next dummy name, for inserting extra rules
  string getDummyName(llvm::Instruction *I);

  // Get the next tmp name, for unsaved returns
  // This will cause the tmp name to be added into locals (otherwise invalid)
  VarName getUnsavedName(llvm::CallInst &I);

  // Get the name to use for formal argument
  // Also creates global exchange var
  string getFormalArgName(llvm::Argument &A);

  // Get local names for a specific function
  set<llvm::Value*> getLocalValues(llvm::Function &F);

  bool runOnModule(llvm::Module &M) override;
  void visitLoadInst(llvm::LoadInst &I);
  void visitAllocaInst(llvm::AllocaInst &I);
  void visitCallInst(llvm::CallInst &I);
  void visitStoreInst(llvm::StoreInst &I);
  void visitBinaryOperator(llvm::BinaryOperator &I);
  void visitPtrToIntInst(llvm::PtrToIntInst &I);
  void visitIntToPtrInst(llvm::IntToPtrInst &I);
  void visitPHINode(llvm::PHINode &I);
  void visitSelectInst(llvm::SelectInst &I);
  void visitGetElementPtrInst(llvm::GetElementPtrInst &I);

  // Should we track this value?
  // returns true for yes, false for no
  VarType resolveType(llvm::Value *V);
  bool filter(llvm::Type *T);
 
  map<int, vn_t> getErrorNames();

  // For program2vec
  std::map<std::string, std::set<std::string>> get_bootstrap_functions();

  // Get an approximate name for GEP based on struct type
  mem_t getApproxName(llvm::GetElementPtrInst &I);

  mem_t getLoadIndex(const llvm::Value *v) const;

private:
  // Core name maps
  map<const llvm::Value*, vn_t> names;
  map<int, vn_t> error_names;

  // The memory model is indexed by MemoryNames (GEP indexes)
  map<VarName, vn_t>      memory_model;

  // Load -> memory model index
  map<const llvm::Value*, mem_t> load_index;

  map<llvm::Function*, VarName> return_names;
  
  // Values that we know are not ECs in a block
  map<llvm::BasicBlock*, set<llvm::Value*>> safe_values;

  map<llvm::Function*, set<llvm::Value*>> locals;

  vn_t EC_OK = make_shared<ErrorName>("OK");

  map<llvm::Instruction*, string> stack_iids;
  unsigned stack_cnt = 1; 

  unsigned dummy_cnt = 1;
  unsigned intermediate_cnt = 1;

  string getIID(llvm::Value *V);
  string generateIntermediateName();

  llvm::Module *module;

  // For converting a Value to a constant struct
  // Used to get global struct literals
  // Returns null if the conversion fails
  llvm::ConstantStruct* getStruct(llvm::Value *V);
  
  // Creates a function MultiName in memory_model
  void backFunction(MemoryName index, llvm::Function *f);

  void updateMemory(MemoryName index, vn_t name);

  void setupFunction(llvm::Function *f);

  queue<llvm::Function*> worklist;

  // Explicity set by mining rather than on command line
  string ecpath_arg;

  std::string getApproxName(llvm::StructType *t);
};

#endif
