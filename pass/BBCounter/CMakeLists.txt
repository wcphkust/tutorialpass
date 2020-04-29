add_library(BBCounterPass BBCounter.cpp ../../include/LiveVariableInLoop.h)

target_compile_features(BBCounterPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(BBCounterPass PROPERTIES
        COMPILE_FLAGS "-fno-rtti -fPIC")
