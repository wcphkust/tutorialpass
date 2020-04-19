cmake_minimum_required(VERSION 3.7)
project(hello-transformation)

find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${LLVM_CLANG_DIRS})

add_subdirectory(pass/OpcodeCounter)
add_subdirectory(pass/BBCounter)
add_subdirectory(pass/BBinLoopCounter)
add_subdirectory(pass/VariableInBB)
add_subdirectory(pass/LiveVariableInBB)
add_subdirectory(pass/LiveVariableInBranch)
add_subdirectory(pass/ParameterCounter)
# add_subdirectory(pass/TypeAnalysis)
