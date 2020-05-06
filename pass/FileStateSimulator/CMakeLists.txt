add_library(FileStateSimulatorPass MODULE FileStateSimulator.cpp)

target_compile_features(FileStateSimulatorPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(FileStateSimulatorPass PROPERTIES COMPILE_FLAGS "-fno-rtti -fPIC")