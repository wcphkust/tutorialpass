#cmake_minimum_required(VERSION 3.15.3)
#project(DFS)
#
##===============================================================================
## 1. LOAD LLVM CONFIGURATION
##===============================================================================
## Set this to a valid LLVM installation dir
#set(LT_LLVM_INSTALL_DIR "/Users/rando/Documents/native-code/llvm-9.0.0.obj")
#
## Add the location of LLVMConfig.cmake to CMake search paths (so that
## find_package can locate it)
#list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")
#
#find_package(LLVM 9.0.0 REQUIRED CONFIG)
#
## HelloWorld includes headers from LLVM - update the include paths accordingly
#include_directories(${LLVM_INCLUDE_DIRS})
#
##===============================================================================
## 2. LLVM-TUTOR BUILD CONFIGURATION
##===============================================================================
## Use the same C++ standard as LLVM does
#set(CMAKE_CXX_STANDARD 14 CACHE STRING "")
#
## LLVM is normally built without RTTI. Be consistent with that.
#if(NOT LLVM_ENABLE_RTTI)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
#endif()

#===============================================================================
# 3. ADD THE TARGET
#===============================================================================
add_library(PropertySimulatorPass PropSim.cpp)
target_compile_features(PropertySimulatorPass PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(PropertySimulatorPass PROPERTIES COMPILE_FLAGS "-fno-rtti -fPIC")



## Allow undefined symbols in shared objects on Darwin (this is the default
## behaviour on Linux)
#target_link_libraries(psa
#        "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")

#add_executable(esp PropSim.cpp)
#
#llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES ${LLVM_TARGETS_TO_BUILD}
#        passes
#        )
#
#target_link_libraries(esp ${REQ_LLVM_LIBRARIES})
#
