#### GOOGLETEST ###
### Tests ####

set(TEST_TOOL_FILES
        ../src/cpp/Llvm.cpp
        ../src/cpp/Path.cpp
        ../src/cpp/Utility.cpp
        ../src/cpp/Context.cpp
        )

set(CLANG_COMMAND clang -c -g -emit-llvm)
add_custom_target(test_bc_bootstrap COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/bootstrap.c)
add_custom_target(test_bc_errpath1 COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/errpath1.c)
add_custom_target(test_bc_errpath2 COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/errpath2.c)
add_custom_target(test_bc_errpath_assign_in_branch COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/errpath_assign_in_branch.c)
add_custom_target(test_bc_errpath_both COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/errpath_both.c)
add_custom_target(test_bc_errpath_early COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/errpath_early.c)
add_custom_target(test_bc_errpath_motivating COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/errpath_motivating.c)
add_custom_target(test_bc_errpath_multi COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/errpath_multi.c)
add_custom_target(test_bc_errpath_return_neither COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/errpath_return_neither.c)
add_custom_target(test_bc_in_loop COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/in_loop.c)
add_custom_target(test_bc_invalid_paths COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/invalid_paths.c)
add_custom_target(test_bc_loop_after COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/loop_after.c)
add_custom_target(test_bc_loop_before COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/loop_before.c)
add_custom_target(test_bc_original COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/original.c)
add_custom_target(test_bc_stop_chain_at_k COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/stop_chain_at_k.c)
add_custom_target(test_bc_struct COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/struct.c)
add_custom_target(test_bc_trivial COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/trivial.c)
add_custom_target(test_bc_recursive COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/recursive.c)
add_custom_target(test_bc_struct2 COMMAND ${CLANG_COMMAND} ${CMAKE_SOURCE_DIR}/tests/programs/struct2.c)
add_custom_target(test_bitcode_files DEPENDS
        test_bc_bootstrap
        test_bc_errpath1
        test_bc_errpath2
        test_bc_errpath_assign_in_branch
        test_bc_errpath_both
        test_bc_errpath_early
        test_bc_errpath_motivating
        test_bc_errpath_multi
        test_bc_errpath_return_neither
        test_bc_in_loop
        test_bc_invalid_paths
        test_bc_loop_after
        test_bc_loop_before
        test_bc_original
        test_bc_stop_chain_at_k
        test_bc_struct
        test_bc_trivial
        test_bc_recursive
        test_bc_struct2
        )

add_executable(runtests ${TEST_TOOL_FILES} FullProgramTest.cpp)

# Now simply link against gtest or gtest_main as needed. Eg
target_link_libraries(runtests gtest_main gmock_main llvmpasses)
add_dependencies(runtests test_bitcode_files)

