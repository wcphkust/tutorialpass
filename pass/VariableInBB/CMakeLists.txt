add_library(VariableInBBPass VariableInBB.cpp)
#add_library(VariableInBB2Pass MODULE VariableInBB.cpp)

target_compile_features(VariableInBBPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(VariableInBBPass PROPERTIES
        COMPILE_FLAGS "-fno-rtti -fPIC")

