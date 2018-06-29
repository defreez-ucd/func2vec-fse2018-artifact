#include <Llvm.hpp>
#include <edgelist.pb.h>
#include <boost/program_options.hpp>

using namespace std;

void print_edgelist(FlowGraph &FG, bool use_ints);

void print_edgelist_protobuf(FlowGraph &FG, const std::unordered_map<int, std::string> &id_to_label);

// Just dumps the graph. We transform in the walker.
int main(int argc, char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
      ("help", "produce help message")
      ("bitcode", po::value<string>()->required(), "Path to bitcode file")
      ("edgelist", po::bool_switch(), "Output labeled edgelist")
      ("protobuf", po::bool_switch(), "Use binary protobuf format")
      ("remove-cross-folder", po::bool_switch(), "Remove cross-folder call edges coming from points-to analysis.")
      ("output", po::value<string>())
      ("error-codes", po::value<string>(), "Path to error codes file");
  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (po::error &e) {
    cerr << "ERROR: " << e.what() << endl << endl;
    cerr << desc << endl;
    return 1;
  }

  string error_codes;
  if (vm.count("error-codes")) {
    error_codes = vm["error-codes"].as<string>();
  }

  bool remove_cross_folder = false;
  if (vm["remove-cross-folder"].as<bool>()) {
    remove_cross_folder = true;
  }

  // Get the ICFG
  p2v::Llvm passes(vm["bitcode"].as<string>(), error_codes, remove_cross_folder);
  shared_ptr<FlowGraph> FG = passes.getFlowGraph();
  if (!FG) {
    throw "Empty ICFG";
  }

  if (vm["edgelist"].as<bool>()) {
    if (vm["protobuf"].as<bool>()) {
      print_edgelist_protobuf(*FG, passes.id_to_label);
    } else {
      print_edgelist(*FG, vm["int"].as<bool>());
    }
  }

  return 0;
}

// TODO: Some decisions are made here about what gets labeled.
// Make it easy to keep print_egelist_protobuf synchronized with print_edgelist
void print_edgelist_protobuf(FlowGraph &FG, const std::unordered_map<int, std::string> &id_to_label) {
  func2vec::Edgelist edgelist;

  BGL_FORALL_EDGES(e, FG.G, _FlowGraph) {
      FlowVertex &source = FG.G[boost::source(e, FG.G)];
      FlowVertex &target = FG.G[boost::target(e, FG.G)];
      func2vec::Edgelist_Edge *edge = edgelist.add_edge();
      edge->set_source(source.stack);
      edge->set_target(target.stack);

      if (FG.G[e].may_ret) {
        edge->set_label("may_ret");
      } else if (FG.G[e].call) {
        edge->set_label("call");
      } else if (FG.G[e].ret) {
        edge->set_label("ret");
      } else if (!source.label_ids.empty()) {
        for (const auto &id : source.label_ids) {
          edge->add_label_id(id);
        }
      }

      // Location of source node
      if (!source.loc.empty()) {
        edge->set_location(source.loc.str());
      }

      assert(edge->label_id_size() == 0 || edge->label().empty());
    }

  auto *m = edgelist.mutable_id_to_label();
  for (const auto &kv : id_to_label) {
    func2vec::Edgelist_Label label;
    label.set_label(kv.second);
    bool ok = m->insert({kv.first, label}).second;
    assert(ok);
  }

  edgelist.SerializeToOstream(&cout);

  google::protobuf::ShutdownProtobufLibrary();
}

// TODO: const-safe way of retrieving vertices
// Output format is meant to match networkx.read_edgelist
void print_edgelist(FlowGraph &FG, bool use_ints) {
  map<string, int> stack_to_int;

  BGL_FORALL_EDGES(e, FG.G, _FlowGraph) {
      FlowVertex &source = FG.G[boost::source(e, FG.G)];
      FlowVertex &target = FG.G[boost::target(e, FG.G)];

      cout << source.stack << " " << target.stack;
      if (FG.G[e].may_ret) {
        cout << " may_ret";
      } else if (FG.G[e].call) {
        cout << " call";
      } else if (FG.G[e].ret) {
        cout << " ret";
      }

      if (!source.loc.empty()) {
        cout << " " << source.loc;
      }

      cout << endl;
    }
}
