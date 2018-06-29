#include "ControlFlow.hpp"
#include <llvm/IR/CFG.h>
#include <llvm/IR/InstIterator.h>

using namespace llvm;
using namespace std;
using namespace ep;

static llvm::RegisterPass<ControlFlowPass> C("control-flow", "Build CFG", false, false);
static cl::opt<string> WriteDot("dot-epcfg", cl::desc("Write dot file for EP CFG"));
static cl::opt<string> DotStart("dot-start", cl::desc("(Optional) function to start dot file at"));

bool ControlFlowPass::runOnModule(Module &M) {
  names = &getAnalysis<NamesPass>();

  Function *main = M.getFunction("main");
  FlowVertex main_v("main.0", main);
  FG.add(main_v);

  if (main) {
    BasicBlock &entry = main->getEntryBlock();
    string entry_name;
    tie(entry_name, std::ignore) = names->getBBNames(entry);
    FlowVertex entry_v(entry_name, main);
    FlowGraph::add_t added = FG.add(main_v, entry_v);
    fn2vtx[main] = std::get<0>(added);
  }

  for (Module::iterator f = M.begin(), e = M.end(); f != e; ++f) {
    if (f->isIntrinsic() || f->isDeclaration()) {
      continue;
    }

    FlowVertex fn_v(names->getCallName(*f), &*f);
    if (!main) {
      FlowGraph::add_t added = FG.add(main_v, fn_v);

      flow_edge_t edge_from_main = std::get<3>(added);
      FG.G[edge_from_main].main = true;

      flow_vertex_t fn_entry = std::get<1>(added);
      fn2vtx[&*f] = fn_entry;
    }

    // Connect function to first basic block
    BasicBlock &entry = f->getEntryBlock();
    string bbe;
    tie(bbe, std::ignore) = names->getBBNames(entry);
    FlowVertex entry_v(bbe, &*f);
    FG.add(fn_v, entry_v);

    runOnFunction(&*f);
  }

  for (Module::iterator f = M.begin(), e = M.end(); f != e; ++f) {
    addMayReturnEdges(*f);
  }

  if (!WriteDot.empty()) {
    write_dot(WriteDot);
  }

  return false;
}

flow_vertex_t ControlFlowPass::getFunctionVertex(const llvm::Function *F) const {
  return fn2vtx.at(F);
}

void ControlFlowPass::write_dot(const string path) {
  ofstream out(path);

  if (!DotStart.empty()) {
    string start_stack = DotStart + ".0";
    FG.write_graphviz(out, start_stack);
  } else {
    FG.write_graphviz(out);
  }

  out.close();
}

// We are building with -fno-exceptions, which means we have to do *something*
// if BOOST throws an exception. We cannot handle it gracefully.
void boost::throw_exception(std::exception const &e) {
  errs() << e.what() << "\n";
  abort();
}

void ControlFlowPass::runOnFunction(Function *F) {
  for (auto bi = F->begin(), be = F->end(); bi != be; ++bi) {
    string bb_enter, bb_exit;
    tie(bb_enter, bb_exit) = names->getBBNames(*bi);

    // Connect exit of each predecessor block to entry of this block
    FlowVertex bbe_v(bb_enter, F);
    for (auto pi = pred_begin(&*bi), pe = pred_end(&*be); pi != pe; ++pi) {
      BasicBlock *pred = *pi;
      string pred_exit;
      tie(std::ignore, pred_exit) = names->getBBNames(*pred);
      FlowVertex predx_v(pred_exit, F);
      FG.add(predx_v, bbe_v);
    }

    BasicBlock *bb = &*bi;
    FlowVertex prev = bbe_v;
    for (auto ii = bb->begin(), ie = bb->end(); ii != ie; ++ii) {
      Instruction *i = &*ii;
      prev = visitInstruction(i, prev);

      FlowGraph::add_t added;
      if (i == bb->getTerminator()) {
        FlowVertex bbx_v(bb_exit, F);
        added = FG.add(prev, bbx_v);
      }

      // Populate fn2ret
      if (isa<ReturnInst>(i)) {
        flow_vertex_t ret_vtx = std::get<1>(added);
        if (!ret_vtx) {
//          cerr << "WARNING : ControlFlowPass::visitInstruction received null return vertex\n";
          return;
        }
        fn2ret[F] = ret_vtx;
      }
    }
  }
}

void ControlFlowPass::addMayReturnEdges(Function &F) {
  if (F.isIntrinsic() || F.isDeclaration()) {
    return;
  }

  string stack = names->getCallName(F);
  flow_vertex_t vtx = FG.getVertex(stack);

  if (!vtx) {
//    cerr << "WARNING: ControlFlowPass::addMayReturnEdges unable to find vertex for stack\n";
    return;
  }

  vector<flow_vertex_t> return_sites;
  flow_in_edge_iter iei, iei_end;
  for (tie(iei, iei_end) = in_edges(vtx, FG.G); iei != iei_end; ++iei) {
    FlowEdge e = FG.G[*iei];
    if (e.call) {
      flow_vertex_t call_site = source(*iei, FG.G);

      // We add an edge to the vertex immediately after call site
      flow_out_edge_iter oei, oei_end;
      for (tie(oei, oei_end) = out_edges(call_site, FG.G); oei != oei_end; ++oei) {
        if (FG.G[*oei].ret) {
          flow_vertex_t ret_to = target(*oei, FG.G);
          return_sites.push_back(ret_to);
          break;
        }
      }
    }
  }

  // Goto return inst for function
  auto ret_vtx_it = fn2ret.find(&F);
  if (ret_vtx_it == fn2ret.end()) {
//    cerr << "WARNING: ControlFlowPass::addMayReturnEdges unable to find return vertex\n";
    return;
  }
  flow_vertex_t ret_from = ret_vtx_it->second;

  for (flow_vertex_t ret_to : return_sites) {
    bool success;
    flow_edge_t may_ret;
    tie(may_ret, success) = boost::add_edge(ret_from, ret_to, FG.G);
    FG.G[may_ret].may_ret = true;
    if (!success) {
//      cerr << "WARNING: ContolFlowPass::addMayReturnEdges failed to add return edge\n";
      return;
    }
  }
}

FlowVertex ControlFlowPass::visitInstruction(Instruction *I, FlowVertex prev) {
  if (!I) {
    cerr << "FATAL ERROR: ControlFlowPass::visitInstruction called with null instruction\n";
    abort();
  }

  if (isa<IntrinsicInst>(I)) {
    return prev;
  }

  string iid = names->getStackName(*I);

  FlowVertex i_v(iid, getSource(I), I);
  // Add the vertex. Be careful of the insanity of FlowGraph properties being overwritten if
  // add is called on vertices that already exist.
  FlowGraph::add_t added = FG.add(prev, i_v);

  flow_vertex_t from = std::get<0>(added);

  // CallInst are not terminators, so all calls are guaranteed to be visited as prev
  if (prev.I && isa<CallInst>(prev.I)) {
    addCalls(from, i_v);
  }

  return i_v;
}

void ControlFlowPass::addCalls(flow_vertex_t call_desc, FlowVertex ret_v) {
  FlowVertex call_v = FG.G[call_desc];
  CallInst *call = dyn_cast<CallInst>(call_v.I);
  if (!call) abort();

  Value *cv = call->getCalledValue();
  if (!cv) return;

  Function *f = call->getCalledFunction();
  if (f && f->isDeclaration()) {
    string call_name = f->getName().str();
    FlowVertex callee_v(call_name, call->getParent()->getParent());
    FG.add(call_v, callee_v, ret_v);
    return;
  }

  // Populate the memory index for indirect calls
  // This is for the word2vec project, but it might
  // be useful for something else.
  bool multi = false;
  mem_t load_index = names->getLoadIndex(cv);
  if (load_index) {
    multi = true;
    call_v.mem_index = load_index;
  }

  vn_t callee_name = names->getVarName(cv);
  if (!callee_name) return;

  mul_t callees;
  if (callee_name->type == VarType::MULTI) {
    if (!multi) {
//      cerr << "WARNING: Inconsistent MULTINAME when adding calls." << endl;
      return;
    }
    callees = static_pointer_cast<MultiName>(callee_name);
  } else {
    callees = make_shared<MultiName>();
    callees->insert(callee_name);
  }

  for (vn_t callee_vn : callees->names()) {
    if (callee_vn->type != VarType::FUNCTION) {
      // See test102, could be function pointer declaration we don't have
      // Nothing we can do
      return;
    }

    fn_t callee_fn = static_pointer_cast<FunctionName>(callee_vn);
    Function *callee = callee_fn->function;

    // ==============================================
    // FIXME!: HACK put this somewhere, anywhere else.
    // If the mem_index is populated, then this came from the points-to analysis.
    // FIXME: Use boost. Handle absolute vs. not absolute paths.
    bool add_edge = true;
    if (remove_cross_folder && call_v.mem_index) {
      string file1, file2;
      string component_f1, component_f2;

      DILocation *loc = nullptr;
      Function *f = callee_fn->function;
      // Get location of the calling function
      assert(call_v.I);
      f = call_v.I->getParent()->getParent();
      // Refactor this loop into a function that can be used by definedfunctions pass and here.
      for (inst_iterator I = inst_begin(f), E = inst_end(f); I != E; ++I) {
        if (loc = I->getDebugLoc()) {
          file1 = loc->getFilename();
          if (file1.find('.') == 0) {
            file1 = file1.substr(2);
          }
          auto idx = file1.find('/');
          component_f1 = file1.substr(0, idx);
          break;
        }
      }

      // Get location of the called function
      f = callee_fn->function;
      for (inst_iterator I = inst_begin(f), E = inst_end(f); I != E; ++I) {
        if (loc = I->getDebugLoc()) {
          file2 = loc->getFilename();
          if (file2.find('.') == 0) {
            file2 = file2.substr(2);
          }
          auto idx = file2.find('/');
          component_f2 = file2.substr(0, idx);
          break;
        }
      }

      if (component_f1 != component_f2 && component_f1 != "include" && component_f2 != "include") {
        add_edge = false;
      }
    }
    // ==============================================


    string call_name = names->getCallName(*callee);
    FlowVertex callee_v(call_name, call->getParent()->getParent());
    if (add_edge) {
      FG.add(call_v, callee_v, ret_v);
    }
  }
}

void ControlFlowPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<NamesPass>();
  AU.setPreservesAll();
}

char ControlFlowPass::ID = 0;
