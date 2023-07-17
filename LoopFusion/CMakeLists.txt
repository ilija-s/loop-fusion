add_library(LoopFusion MODULE
	# List of source files
	LoopFusion.cpp
)

target_compile_features(LoopFusion PRIVATE cxx_std_17)

set_target_properties(LoopFusion PROPERTIES
	COMPILE_FLAGS "-fno-rtti"
)

