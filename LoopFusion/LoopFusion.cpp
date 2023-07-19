#include "FusionCandidate.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {

using FusionCandidatesTy = SmallVector<FusionCandidate>;
// TODO: Implement sorting of FusionCandidates that are CFE, they need to be
// ordered in respect to their dominance order.
using CFESetsTy = SmallVector<std::set<FusionCandidate>>;

struct LoopFusion : public FunctionPass {
  FusionCandidatesTy FusionCandidates;
  CFESetsTy CFESets;
  static char ID; // Pass identification, replacement for typeid

  LoopFusion() : FunctionPass(ID) {}

  bool haveSameTripCounts(Loop *L1, Loop *L2, ScalarEvolution &SE) {
    const SCEV *TripCount1 = SE.getBackedgeTakenCount(L1);
    if (isa<SCEVCouldNotCompute>(TripCount1)) {
      dbgs() << "Trip count can not be computed.";
      return false;
    }
    dbgs() << *TripCount1 << '\n'; // Debug print.

    const SCEV *TripCount2 = SE.getBackedgeTakenCount(L2);
    if (isa<SCEVCouldNotCompute>(TripCount2)) {
      dbgs() << "Trip count can not be computed.";
      return false;
    }
    dbgs() << *TripCount2 << '\n'; // Debug print.

    return (TripCount1 == TripCount2);
  }

  /// Do all checks to figure out if loops can be fused.
  bool CanFuseLoops(FusionCandidate *L1, FusionCandidate *L2,
                    ScalarEvolution &SE) {
    return haveSameTripCounts(L1->getLoop(), L2->getLoop(), SE);
  }

  /// Function that will fuse loops based on previously established candidates.
  void FuseLoops(FusionCandidate *L1, FusionCandidate *L2) {
    // Create a new loop with a combined loop bound that covers the iterations
    // of both loops being fused.

    // Modify the loop body to incorporate the instructions from both loops.
    // We need to ensure that the instructions are ordered correctly to
    // maintain correct program semantics.

    // Update loop-carried dependencies, such as induction variables, to reflect
    // the new loop structure.

    // Remove the original loops from the LLVM IR.
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<PostDominatorTreeWrapperPass>();

    AU.addPreserved<LoopInfoWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
    AU.addPreserved<ScalarEvolutionWrapperPass>();
    AU.addPreserved<PostDominatorTreeWrapperPass>();
  }

  bool runOnFunction(Function &F) override {
    // for each loop L: LoopInfo analysis pass is needed
    //    collect fusion candidates - Use FusionCandidate class to determine
    //    sort candidates into control-flow equivalent sets - impl comparison
    //    for each CFE set:
    //      for each pair of loops Li Lj
    //        if (CanFuseLoops(Li, Lj)):
    //          FuseLoops(Li, Lj)

    auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    auto &PDT = getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

    // Collect fusion candidates.
    SmallVector<Loop *> FunctionLoops(LI.begin(), LI.end());
    for (const auto &L : FunctionLoops) {
      dbgs() << "Number of basic blocks: " << L->getNumBlocks() << "\n";
      FusionCandidate FC(L);
      if (FC.isCandidateForFusion()) {
        FusionCandidates.emplace_back(FC);
      }
    }

    if(CanFuseLoops(&FusionCandidates[0], &FusionCandidates[1])) {
        FuseLoops(&FusionCandidates[0], &FusionCandidates[1]);
    }

    return true;
  }
};
} // namespace

char LoopFusion::ID = 0;
static RegisterPass<LoopFusion> X("loopfusion", "Loop Fusion Pass");
