#include "FusionCandidate.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

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
    BasicBlock *Loop1Header = L1->getHeader();
    int Loop1Bound = -1;
    bool IsLoop1BoundConstant = false;

    for (Instruction &Instr : *Loop1Header) {
      if (isa<ICmpInst>(&Instr)) {
        if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr.getOperand(1))) {
          Loop1Bound = ConstInt->getSExtValue();
          IsLoop1BoundConstant = true;
        }
      }
    }

    BasicBlock *Loop2Header = L2->getHeader();
    int Loop2Bound = -1;
    bool IsLoop2BoundConstant = false;

    for (Instruction &Instr : *Loop2Header) {
      if (isa<ICmpInst>(&Instr)) {
        if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr.getOperand(1))) {
          Loop2Bound = ConstInt->getSExtValue();
          IsLoop2BoundConstant = true;
        }
      }
    }

    if(IsLoop1BoundConstant && IsLoop2BoundConstant) {
      return Loop1Bound == Loop2Bound;
    }
    return false;
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
      AU.addRequiredID(LoopSimplifyID);
      AU.addRequired<LoopInfoWrapperPass>();
      AU.addRequired<DominatorTreeWrapperPass>();
      AU.addRequired<ScalarEvolutionWrapperPass>();
      AU.addRequired<PostDominatorTreeWrapperPass>();

      AU.setPreservesAll();
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
      dbgs() << "\nNumber of basic blocks: " << L->getNumBlocks() << "\n";
      auto Preheader = L->getLoopPreheader();
      auto Header = L->getHeader();
      auto ExitingBlock = L->getExitingBlock();
      auto ExitBlock = L->getExitBlock();
      auto Latch = L->getLoopLatch();
      dbgs() << "\n"
             << "\tPreheader: "
             << (Preheader ? Preheader->getName() : "nullptr") << "\n"
             << "\tHeader: " << (Header ? Header->getName() : "nullptr") << "\n"
             << "\tExitingBB: "
             << (ExitingBlock ? ExitingBlock->getName() : "nullptr") << "\n"
             << "\tExitBB: " << (ExitBlock ? ExitBlock->getName() : "nullptr")
             << "\n"
             << "\tLatch: " << (Latch ? Latch->getName() : "nullptr") << "\n"
             << "\n";

      dbgs() << "HAVE SAME TRIP COUNTS:" << haveSameTripCounts(FunctionLoops[0], FunctionLoops[1], SE) << '\n';

      if (!SE.hasLoopInvariantBackedgeTakenCount(L)) {
        dbgs() << "Loop " << L->getName() << " trip count not computable\n";
      }          //LoopCounter = VariablesMap[Instr.getOperand(0)];

      if (!L->isLoopSimplifyForm()) {
        dbgs() << "Loop " << L->getName() << " is not in simplified form\n";
      }
      FusionCandidate FC(L);
      if (FC.isCandidateForFusion()) {
        FusionCandidates.emplace_back(FC);
      }
    }

    if (CanFuseLoops(&FusionCandidates[0], &FusionCandidates[1], SE)) {
      FuseLoops(&FusionCandidates[0], &FusionCandidates[1]);
    }

    return true;
  }
};
} // namespace

char LoopFusion::ID = 0;
static RegisterPass<LoopFusion> X("loopfusion", "Loop Fusion Pass");
