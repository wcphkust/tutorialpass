add_library(LiveVariableViaBBPass MODULE LiveVariableViaBB.cpp)
target_link_libraries(LiveVariableViaBBPass VariableInBBPass)

target_compile_features(LiveVariableViaBBPass PRIVATE cxx_range_for cxx_auto_type)


# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(LiveVariableViaBBPass PROPERTIES COMPILE_FLAGS "-fno-rtti -fPIC")