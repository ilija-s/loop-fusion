add_llvm_library( LLVMLoopFusion MODULE
  LoopFusion.cpp

  PLUGIN_TOOL
  opt
  )
