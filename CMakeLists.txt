cmake_minimum_required(VERSION 3.7)
project(hello-transformation)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})

include_directories(include)

include_directories(${LLVM_INCLUDE_DIRS}) # include
link_directories(${LLVM_LIBRARY_DIRS}) # libs

add_subdirectory(util)
add_subdirectory(pass)
add_subdirectory(statistic)


#if(APPLE)
#    add_subdirectory(csachecker/TypeAnalysis)
#endif(APPLE)



