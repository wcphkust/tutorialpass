cmake_minimum_required(VERSION 3.7)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})

include_directories(include)

include_directories(${LLVM_INCLUDE_DIRS}) # include
link_directories(${LLVM_LIBRARY_DIRS}) # libs


add_library(VirtualCallAnalysisPass MODULE VirtualCallAnalysis.cpp)

target_compile_features(VirtualCallAnalysisPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(VirtualCallAnalysisPass PROPERTIES COMPILE_FLAGS "-fno-rtti")

if(APPLE)
    set_target_properties(VirtualCallAnalysisPass PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)