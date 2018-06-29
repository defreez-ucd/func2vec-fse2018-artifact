project(func2vec)
set(func2vec_VERSION_MAJOR 0)
set(func2vec_VERSION_MINOR 1)

cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)

include_directories(include)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS} ${COMPILE_FLAGS})
link_directories(${LLVM_INSTALL_PREFIX}/lib)

find_package(Boost COMPONENTS program_options REQUIRED)

set(TOOL_FILES
        src/cpp/Llvm.cpp
        src/cpp/Utility.cpp
        )
set(PATHGEN_FILES
        src/cpp/pathgen.cpp
        src/cpp/Path.cpp
        src/cpp/Context.cpp
        ${TOOL_FILES}
        )
set(TRACEGEN_FILES
        src/tracegen/main.cpp
        src/tracegen/TraceDatabase.cpp
        src/tracegen/Traces.cpp
        src/tracegen/TraceVisitors.cpp
        src/cpp/Utility.cpp
        )
set(GETGRAPH_FILES
        src/cpp/getgraph.cpp
        src/cpp/edgelist.pb.cc
        ${TOOL_FILES}
        )
set(PASS_FILES
        src/passes/Names.cpp
        src/passes/ControlFlow.cpp
        src/passes/Handlers.cpp
        src/passes/BranchSafety.cpp
        src/cpp/Utility.cpp
        src/passes/DefinedFunctions.cpp
        src/passes/SourceInfo.cpp
        src/passes/WrapperFunctions.cpp
        src/passes/IfStatements.cpp
        src/passes/InstructionLabels.cpp
        src/passes/ErrorCodeInstructions.cpp
        src/passes/FlatFunctions.cpp
        )

# This cannot be a shared library because LLVM uses globals for options.
# The MODULE target can be loaded by opt on the command line.
# The SHARED target is linked into main.
add_library(mypasses MODULE ${PASS_FILES})
add_library(llvmpasses SHARED ${PASS_FILES})
set_target_properties(mypasses PROPERTIES COMPILE_FLAGS -fno-exceptions)
set_target_properties(llvmpasses PROPERTIES COMPILE_FLAGS -fno-exceptions)

llvm_map_components_to_libnames(llvm_libs support core irreader analysis)
target_link_libraries(llvmpasses ${llvm_libs})

# pathgen
add_executable(pathgen ${PATHGEN_FILES})
add_dependencies(pathgen llvmpasses)
target_link_libraries(pathgen llvmpasses)

# tracegen
add_executable(tracegen ${TRACEGEN_FILES})
target_link_libraries(tracegen llvmpasses sqlite3)

# getgraph
add_executable(getgraph ${GETGRAPH_FILES})
add_dependencies(getgraph llvmpasses)
target_link_libraries(getgraph llvmpasses ${Boost_LIBRARIES} protobuf)

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
if (result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif ()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
if (result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif ()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
        ${CMAKE_BINARY_DIR}/googletest-build)

add_subdirectory(tests)

# Eclat for miner
include(ExternalProject)
ExternalProject_Add(ECLAT
        CONFIGURE_COMMAND ""
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/eclat/eclat/src
        BUILD_COMMAND $(MAKE)
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE 1
        )

# Clang plugin for identifying error handlers
#add_library(plugin_errhandlers MODULE src/clang/PluginErrhandlers.cpp)
#target_link_libraries(plugin_errhandlers clangASTMatchers)
