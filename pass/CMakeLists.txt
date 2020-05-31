cmake_minimum_required(VERSION 3.7)
project(hello-transformation)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})

include_directories(include)

include_directories(${LLVM_INCLUDE_DIRS}) # include
link_directories(${LLVM_LIBRARY_DIRS}) # libs

add_subdirectory(FileStateSimulator)
add_subdirectory(VariableInBB)
add_subdirectory(BBinLoopCounter)
add_subdirectory(BBCounter)
add_subdirectory(InterSignAnalysis)
add_subdirectory(LiveVariableInBB)
add_subdirectory(LiveVariableViaInst)
add_subdirectory(LiveVariableViaBB)
add_subdirectory(OpcodeCounter)
add_subdirectory(ParameterCounter)
add_subdirectory(VirtualFuncAnalysis)