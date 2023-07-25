#include "FusionCandidate.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
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

  /// Do all checks to figure out if loops can be fused.
  bool CanFuseLoops(Loop *L1, Loop *L2) { 

    // Checking if loops are adjacent
    SmallVector<BasicBlock*> ExitBlocks1;
    SmallVector<BasicBlock*> ExitBlocks2;
    L1->getExitBlocks(ExitBlocks1);
    L2->getExitBlocks(ExitBlocks2);

    return
      ExitBlocks1.size() == 1 && ExitBlocks2.size() <= 1
      && *ExitBlocks1.begin() == L2->getLoopPreheader();

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

    // Debug message
    if (CanFuseLoops(L1->loop(), L2->loop())) {
      dbgs() << "Can fuse loops\n";
    }
    else {
      dbgs() << "Cannot fuse loops\n";
    }
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();

    AU.addPreserved<ScalarEvolutionWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
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
    auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();

    // Collect fusion candidates.
    SmallVector<Loop *> FunctionLoops(LI.begin(), LI.end());
    for (const auto &L : FunctionLoops) {
      dbgs() << "Number of basic blocks: " << L->getNumBlocks() << "\n";
      FusionCandidate FC(L);
      if (FC.isCandidateForFusion()) {
        FusionCandidates.emplace_back(FC);
      }
    }

    // Fuse loops - skip checking if loops can be fused for now.
    FuseLoops(&FusionCandidates[1], &FusionCandidates[0]);
    FuseLoops(&FusionCandidates[2], &FusionCandidates[1]);

    return true;
  }
};
} // namespace

char LoopFusion::ID = 0;
static RegisterPass<LoopFusion> X("loopfusion", "Loop Fusion Pass");
