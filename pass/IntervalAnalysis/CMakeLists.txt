add_library(IntAnalysisPASS MODULE IntervalAnalysis.cpp)

target_compile_features(IntAnalysisPASS PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(IntAnalysisPASS PROPERTIES COMPILE_FLAGS "-fno-rtti -fPIC")
