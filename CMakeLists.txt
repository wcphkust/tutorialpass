cmake_minimum_required(VERSION 3.7)
project(hello-transformation)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories("include")
include_directories(${LLVM_INCLUDE_DIRS}) # include
link_directories(${LLVM_LIBRARY_DIRS}) # libs

if (NOT APPLE)
    add_subdirectory(pass/OpcodeCounter)
    add_subdirectory(pass/BBCounter)
    add_subdirectory(pass/BBinLoopCounter)
    add_subdirectory(pass/VariableInBB)
    add_subdirectory(pass/LiveVariableInBB)
    add_subdirectory(pass/LiveVariableInBranch)
    add_subdirectory(pass/LiveVariableInLoop)
    add_subdirectory(pass/ParameterCounter)
endif(NOT APPLE)

if(APPLE)
    add_subdirectory(csachecker/TypeAnalysis)
endif(APPLE)



