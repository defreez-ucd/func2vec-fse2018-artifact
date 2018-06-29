#include "Context.hpp"

using namespace std;
using namespace p2v;

const unsigned DEFAULT_P = 200;

void usage() {
  cerr << "Usage: main\t-b <bitcode file> -i <interesting functions file> [-e <error codes file>] [-c]" << endl
       << "\t\t[-p <max path length] [-l]" << endl << endl
       << "-c will enable CALLER_ paths." << endl
       << "-e will enable error path annotations." << endl
       << "-r <string> will set early return string (default RETURN_DEFAULT), requires -e" << endl;
}

int main(int argc, char **argv) {
  string arg_bitcode_path, arg_interesting_path, arg_path_length;
  bool arg_bootstrap_output = false, arg_callinfo = false, arg_err_annotations = false;
  string arg_return_str, arg_ec_path;
  unsigned p = DEFAULT_P;

  int c;
  while ((c = getopt(argc, argv, "b:i:p:lce:r:")) != EOF) {
    switch(c) {
    case 'b':
      arg_bitcode_path = optarg;
      break;
    case 'i':
      arg_interesting_path = optarg;
      break;
    case 'p':
      arg_path_length = optarg;
      break;
    case 'l':
      arg_bootstrap_output = true;
      break;
    case 'c':
      arg_callinfo = true;
      break;
    case 'e':
      arg_ec_path = optarg;
      break;
    case 'r':
      arg_return_str = optarg;
      break;
    }
  }

  if (arg_bitcode_path.empty()) {
    cerr << "Bitcode path is empty.\n\n" << endl;
    usage();
    return 1;
  }

  if (!arg_path_length.empty()) {
    istringstream ss(arg_path_length);
    ss >> p;
    if (ss.fail() || p < 1 || p > 1000) {
      cerr << "Unrecognized k value. Please choose 1 <= p <= 1000.\n";
      return 1;
    }
  }

  if (arg_bootstrap_output) {
    // Just print the bootstrap functions and exit
    Llvm passes(arg_bitcode_path);
    map<string, set<string>> bootstrap_functions = passes.getBootstrapFns();
    for (auto i = bootstrap_functions.begin(), e = bootstrap_functions.end(); i != e; ++i) {
      string group_id = i->first;
      replace(group_id.begin(), group_id.end(), '.', '_');
      for (auto j = i->second.begin(), k = i->second.end(); j != k; ++j) {
        string fname = *j;
        auto idx = fname.find('.');
        if (idx != string::npos) {
          fname = fname.substr(0, idx);
        }
        cout << fname << " ";
      }
      cout << endl;
    }
    return 0;
  }

  if (arg_interesting_path.empty()) {
    cerr << "Interesting path is empty.\n\n" << endl;
    usage();
    return 1;
  }
  
  // Generate paths
  string return_str;
  if (!arg_return_str.empty()) {
    return_str = arg_return_str;
  } else {
    return_str = "DEFAULT";
  }
    
  run_k_context_on_file(arg_bitcode_path,
                        arg_interesting_path,
                        cout,
                        p,
                        arg_callinfo,
                        !arg_ec_path.empty(),
                        return_str,
                        arg_ec_path);

  return 0;
}
