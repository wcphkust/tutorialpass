add_library(LiveVariableInBBPass MODULE LiveVariableInBB.cpp)
target_link_libraries(LiveVariableInBBPass VariableInBBPass)

target_compile_features(LiveVariableInBBPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(LiveVariableInBBPass PROPERTIES COMPILE_FLAGS "-fno-rtti")

