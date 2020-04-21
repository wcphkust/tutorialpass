cmake_minimum_required(VERSION 3.7)
project(hello-transformation)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_library(LiveVariableInBranchPass MODULE LiveVariableInBranchSkeleton.cpp)

target_compile_features(LiveVariableInBranchPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(LiveVariableInBranchPass PROPERTIES COMPILE_FLAGS "-fno-rtti")