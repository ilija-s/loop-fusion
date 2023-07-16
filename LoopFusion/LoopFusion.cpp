#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
using namespace llvm;

namespace {
  struct LoopFusion : public FunctionPass {
    std::vector<std::set<Loop *>> FusionCandidates; // Might not be the most suitable data stucture, as Loop equivalence comparison is currently questionable.
  
    static char ID; // Pass identification, replacement for typeid
    LoopFusion() : FunctionPass(ID) {}
    
    std::vector<std::set<Loop *>> FindFusionCandidates(Function *F) { // Function that will fill and sort the FusionCandidates data structure.
    	std::vector<std::set<Loop *>> placeholder;
    	return placeholder;
    }
    
    bool CanFuseLoops(Loop *L1, Loop *L2) { // Do all checks to figure out if loops can be fused.
    	return true;
    }
    
    void FuseLoops(Loop *L1, Loop *L2); // Function that will fuse loops based on previously established candidates.

    bool runOnFunction(Function &F) override {
      FusionCandidates = FindFusionCandidates(&F);
      for (int i=0; i<FusionCandidates.size(); i++) {
      	for (Loop* L1 : FusionCandidates[i]) {
      	  for(Loop* L2 : FusionCandidates[i]) {
      	    if(L1 != L2 && CanFuseLoops(L1, L2)) {
      	      FuseLoops(L1, L2);
      	    }
      	  }
      	}
      }
      return true; // Should return true in the future, as files will be changed.
    }
  };
}

char LoopFusion::ID = 0;
static RegisterPass<LoopFusion> X("loopfusion", "Loop Fusion Pass");
