#ifndef ERRORCODES_H
#define ERRORCODES_H

#include <unordered_set>

// This needs to match errno-base.h in llvmlinux. 
// The purpose of using int64_t is to match llvm::ConstantInt::getLimitedValue()
// Both the positive and negative values are stored for easy matching against constants
// as they are represented in the bitcode.

const std::unordered_set<int64_t> ERROR_CODES =
  { 114, 214, 314, 414, 514,
    614, 714, 814, 914, 1014,
    1114, 1214, 1314, 1414, 1514,
    1614, 1714, 1814, 1914, 2014,
    2114, 2214, 2314, 2414, 2514,
    2614, 2714, 2814, 2914, 3014,
    3114, 3214, 3314, 3414,
    -114, -214, -314, -414, -514,
    -614, -714, -814, -914, -1014,
    -1114, -1214, -1314, -1414, -1514,
    -1614, -1714, -1814, -1914, -2014,
    -2114, -2214, -2314, -2414, -2514,
    -2614, -2714, -2814, -2914, -3014,
    -3114, -3214, -3314, -3414,
  };

#endif
