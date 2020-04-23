cmake_minimum_required(VERSION 2.8.8)

#Project declaration
project(ClangTypeAnalysis DESCRIPTION "libClangTypeAnalysis")

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

#We might only have llvm-config-X.Y
find_program (LLVM_CONFIG_WITH_NUMBER "llvm-config-${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}")
if(LLVM_CONFIG_WITH_NUMBER MATCHES "NOTFOUND")
    find_program (REAL_LLVM_CONFIG "llvm-config")
else()
    set(REAL_LLVM_CONFIG ${LLVM_CONFIG_WITH_NUMBER})
endif()
message(STATUS "Found: ${REAL_LLVM_CONFIG}")

#get compile flags
execute_process(COMMAND "${REAL_LLVM_CONFIG}" --cxxflags  OUTPUT_VARIABLE CPP_LLVM_CONFIG OUTPUT_STRIP_TRAILING_WHITESPACE)

#get system lib link flags
execute_process(COMMAND "${REAL_LLVM_CONFIG}" --system-libs OUTPUT_VARIABLE SYSTEM_LIBS_LLVM_CONFIG OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND "${REAL_LLVM_CONFIG}" --includedir OUTPUT_VARIABLE LLVM_CONFIG_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND "${REAL_LLVM_CONFIG}" --src-root OUTPUT_VARIABLE LLVM_SRC_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND "${REAL_LLVM_CONFIG}" --obj-root OUTPUT_VARIABLE LLVM_OBJ_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
include_directories(SYSTEM ${LLVM_SRC_DIR}/tools/clang/include SYSTEM ${LLVM_OBJ_DIR}/tools/clang/include SYSTEM ${LLVM_CONFIG_INCLUDE_DIR})
message(STATUS "include dir is ${LLVM_CONFIG_INCLUDE_DIR} ${LLVM_OBJ_DIR}/tools/clang/include ${LLVM_CONFIG_INCLUDE_DIR}")

execute_process(COMMAND "${REAL_LLVM_CONFIG}" --libdir OUTPUT_VARIABLE LLVM_CONFIG_LINk_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
link_directories( ${LLVM_CONFIG_LINk_DIR} )
message(STATUS "link dir is ${LLVM_CONFIG_LINk_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${CPP_LLVM_CONFIG}")

add_library(ClangTypeAnalysis SHARED ClangTypeAnalysis.cpp)

set (LLVM_LIBS
        LLVM
        )

set(CLANG_LIBS
        clangBasic
        clangEdit
        clangFrontend
        clangStaticAnalyzerFrontend
        clangStaticAnalyzerCore
        clangStaticAnalyzerCheckers
        clangSerialization
        clangSema
        clangAnalysis
        clangCrossTU
        clangRewriteFrontend
        clangRewrite
        clangLex
        clangAST
        clangASTMatchers
        clangDynamicASTMatchers
        clangIndex
        clangDriver
        clangParse
        )

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(START_GROUP "-Wl,--start-group")
    set(END_GROUP "-Wl,--end-group")
else()
    set(START_GROUP "")
    set(END_GROUP "")
endif()

target_link_libraries(ClangTypeAnalysis
        ${START_GROUP}
        ${CLANG_LIBS}
        ${LLVM_LIBS}
        ${END_GROUP}
        ${SYSTEM_LIBS_LLVM_CONFIG})

# install(TARGETS clang-highlight RUNTIME DESTINATION bin)

# add_custom_target(ClangHighlightUnitTests)
# set_target_properties(ClangHighlightUnitTests PROPERTIES FOLDER "Clang Highlight Unit Tests")

# function(add_highlight_unittest test_dirname)
#   add_unittest(ClangHighlightUnitTests ${test_dirname} ${ARGN})
# endfunction()

# add_subdirectory(unittests)
