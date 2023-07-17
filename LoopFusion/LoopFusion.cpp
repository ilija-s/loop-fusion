#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
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
    // for each loop L: LoopInfo analysis pass is needed
    //    collect fusion candidates - Use FusionCandidate class to determine
    //    sort candidates into control-flow equivalent sets - impl comparison
    //    for each CFE set:
    //      for each pair of loops Li Lj
    //        if (CanFuseLoops(Li, Lj)):
    //          FuseLoops(Li, Lj)
    return true;
  }
};
} // namespace

char LoopFusion::ID = 0;
static RegisterPass<LoopFusion> X("loopfusion", "Loop Fusion Pass");
