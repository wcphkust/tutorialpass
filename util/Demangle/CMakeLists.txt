add_library(DemangleLib demangle.cpp)

target_compile_features(DemangleLib PUBLIC cxx_range_for cxx_auto_type)

set_target_properties(DemangleLib PROPERTIES COMPILE_FLAGS "-fno-rtti")