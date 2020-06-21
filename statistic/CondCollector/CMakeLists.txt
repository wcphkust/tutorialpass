add_library(CondCollectorStatistic MODULE CondCollector.cpp)

target_compile_features(CondCollectorStatistic PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(CondCollectorStatistic PROPERTIES COMPILE_FLAGS "-fno-rtti")

if(APPLE)
    set_target_properties(CondCollectorStatistic PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)