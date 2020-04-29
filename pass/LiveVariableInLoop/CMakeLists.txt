add_library(LiveVariableInLoopPass MODULE LiveVariableInLoop.cpp)
target_link_libraries(LiveVariableInLoopPass VariableInBBPass)

target_compile_features(LiveVariableInLoopPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(LiveVariableInBranchPass PROPERTIES COMPILE_FLAGS "-fno-rtti -fPIC")
include_directories("../../build/VariableInBB")