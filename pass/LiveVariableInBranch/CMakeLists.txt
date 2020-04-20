# CSDN: https://blog.csdn.net/bigdog_1027/article/details/79113342
#add_library(LiveVariableInBBPass MODULE ../VariableInBB/VariableInBB.cpp)
add_library(LiveVariableInBranchPass MODULE LiveVariableInBranch.cpp)
target_link_libraries(LiveVariableInBranchPass VariableInBBPass)

target_compile_features(LiveVariableInBranchPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(LiveVariableInBranchPass PROPERTIES COMPILE_FLAGS "-fno-rtti")
include_directories("../../build/VariableInBB")