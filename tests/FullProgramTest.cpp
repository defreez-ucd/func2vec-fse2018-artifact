#include "test.hpp"
#include "Context.hpp"

using namespace std;

using ::testing::_;
using ::testing::Return;
using ::testing::ContainerEq;

class FullProgramTest : public ::testing::Test {};

// TODO: Run all tests from a single driver

string run_k_context(string source_name, unsigned path_length,
                     bool err_annotations=false,
                     string return_str="") {

  // TODO: Fix paths so these can be run from somewhere other than root directory
  stringstream ss;

  if (err_annotations) {
    run_k_context_on_file(source_name + ".bc",
                          INTERESTING_TXT,
                          ss,
                          path_length,
                          false,
                          err_annotations,
                          return_str,
                          "../../config/codes.txt"
                          );
  } else {
    run_k_context_on_file(source_name + ".bc",
                          INTERESTING_TXT,
                          ss,
                          path_length,
                          false,
                          err_annotations,
                          return_str);
  }
  return ss.str();                        
}

TEST_F(FullProgramTest, Trivial) {
  string res = run_k_context("trivial", 100);
  string expected = "PATH_BEGIN interesting PATH_END\n";
  ASSERT_EQ(res, expected);
}

TEST_F(FullProgramTest, Original) {
  string res = run_k_context("original", 100);
  string expected;
  expected += "PATH_BEGIN func4 foo func6 interesting func7 PATH_END\n";
  expected += "PATH_BEGIN func4 foo func5 interesting func7 PATH_END\n";
  ASSERT_EQ(res, expected);
}

TEST_F(FullProgramTest, LoopAfter) {
  string res = run_k_context("loop_after", 100);
  ASSERT_NE(res.find("PATH_BEGIN interesting PATH_END\n"), string::npos) << res;  // Not entering loop
  ASSERT_NE(res.find("PATH_BEGIN interesting a PATH_END\n"), string::npos) << res; // Entering loop
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 2) << res;
}

TEST_F(FullProgramTest, LoopBefore) {
  string res = run_k_context("loop_before", 100);
  ASSERT_NE(res.find("PATH_BEGIN a interesting PATH_END\n"), string::npos) << res; // Entering loop
  ASSERT_NE(res.find("PATH_BEGIN interesting PATH_END\n"), string::npos) << res;  // Not entering loop
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 2) << res;
}

TEST_F(FullProgramTest, InLoop) {
  string res = run_k_context("in_loop", 100);
  string expected;
  ASSERT_NE(res.find("PATH_BEGIN interesting b PATH_END\n"), string::npos);
  ASSERT_NE(res.find("PATH_BEGIN a interesting b PATH_END\n"), string::npos);

  // FIXME
  // This isn't great. Effectly allows stopping at the loop and not continuing after.  
  ASSERT_NE(res.find("PATH_BEGIN interesting PATH_END\n"), string::npos);
  ASSERT_NE(res.find("PATH_BEGIN a interesting PATH_END\n"), string::npos);
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 4) << res;
}

TEST_F(FullProgramTest, InvalidPaths) {
  string res = run_k_context("invalid_paths", 100);
  string expected; 
  expected += "PATH_BEGIN U interesting X PATH_END\n";
  expected += "PATH_BEGIN A interesting D PATH_END\n";
  ASSERT_EQ(res, expected);
}

TEST_F(FullProgramTest, StopChainAtP) {
  string res = run_k_context("stop_chain_at_k", 2);
  string expected; 
  expected += "PATH_BEGIN c d interesting e f PATH_END\n";
  ASSERT_EQ(res, expected);
}

TEST_F(FullProgramTest, ErrPath1) {
  string res = run_k_context("errpath1", 100, true, "DEFAULT");
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo4 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo4 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo3 RETURN_NO_ERR PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 3) << res;
                     
}

TEST_F(FullProgramTest, ErrPath2) {
  string res = run_k_context("errpath2", 100, true, "DEFAULT");
  ASSERT_NE(res.find("PATH_BEGIN NO_ERR_foo3 foo6 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo3 foo6 RETURN_NO_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo4 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo4 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 4) << res;
}

TEST_F(FullProgramTest, ErrPathMotivating) {
  string res = run_k_context("errpath_motivating", 100, true, "DEFAULT");

  // Ugh, nondeterministic choice of which branch comes first
  ASSERT_NE(res.find("PATH_BEGIN NO_ERR_foo3 foo5 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN NO_ERR_foo3 foo6 PATH_END\n"), string::npos) << res;  
  ASSERT_NE(res.find("PATH_BEGIN NO_ERR_foo5 foo6 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN foo1 interesting foo3 foo5 foo6 RETURN_NO_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN foo1 interesting foo3 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo4 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN foo1 interesting foo4 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo5 PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 8) << res;
}

TEST_F(FullProgramTest, ErrPathBoth) {
  string res = run_k_context("errpath_both", 100, true, "DEFAULT");
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo4 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo6 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN NO_ERR_foo3 foo6 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo3 foo6 RETURN_NO_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo4 foo6 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 5) << res;
}

TEST_F(FullProgramTest, ErrPathMulti) {
  string res = run_k_context("errpath_multi", 100, true, "DEFAULT");
  ASSERT_NE(res.find("PATH_BEGIN interesting foo3 RETURN_NO_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo2 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo2 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo4 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo2 foo4 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo4 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 6) << res;
}

TEST_F(FullProgramTest, ErrPathReturnNeither) {
  string res = run_k_context("errpath_return_neither", 100, true, "DEFAULT");
  ASSERT_NE(res.find("PATH_BEGIN interesting foo4 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo4 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo3 RETURN_NO_ERR PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 3) << res;
}

TEST_F(FullProgramTest, ErrAssignInBranch) {
  string res = run_k_context("errpath_assign_in_branch", 100, true, "DEFAULT");
  ASSERT_NE(res.find("PATH_BEGIN interesting foo4 RETURN_ERR PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN ERR_foo3 foo4 PATH_END\n"), string::npos) << res;
  ASSERT_NE(res.find("PATH_BEGIN interesting foo3 RETURN_NO_ERR PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 3) << res;
}

TEST_F(FullProgramTest, ErrEarly) {
  string res = run_k_context("errpath_early", 100, true, "DEFAULT");
  ASSERT_NE(res.find("PATH_BEGIN interesting foo2 RETURN_DEFAULT PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 1) << res;
}

TEST_F(FullProgramTest, ErrEarlyStr) {
  string res = run_k_context("errpath_early", 100, true, "NO_ERR");
  ASSERT_NE(res.find("PATH_BEGIN interesting foo2 RETURN_NO_ERR PATH_END\n"), string::npos) << res;
  ASSERT_EQ(std::count(res.begin(), res.end(), '\n'), 1) << res;
}
