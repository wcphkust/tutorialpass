add_library(LiveVariableViaInstPass MODULE LiveVariableViaInst.cpp ../../util/WorkList/WorkList.cpp)

target_link_libraries(LiveVariableViaInstPass VariableInBBPass)
target_compile_features(LiveVariableViaInstPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(LiveVariableViaInstPass PROPERTIES COMPILE_FLAGS "-fno-rtti -fPIC")