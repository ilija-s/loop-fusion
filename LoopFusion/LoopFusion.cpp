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
  std::unordered_map<Value *, Value *> VariablesMap;
  CFESetsTy CFESets;
  static char ID; // Pass identification, replacement for typeid

  LoopFusion() : FunctionPass(ID) {}

  void MapVariables(Function *F)
  {
    for (BasicBlock &BB : *F) {
      for (Instruction &Instr : BB) {
        if (isa<LoadInst>(&Instr)) {
          VariablesMap[&Instr] = Instr.getOperand(0);
        }
      }
    }
  }

  bool HaveSameTripCounts(Loop *L1, Loop *L2) {
    BasicBlock *Loop1Header = L1->getHeader();

    Value *Variable1;
    Value *Variable2;

    int Loop1Bound = -1;
    bool IsLoop1BoundConstant = false;

    for (Instruction &Instr : *Loop1Header) {
      if (isa<ICmpInst>(&Instr)) {
        if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr.getOperand(1))) {
          Loop1Bound = ConstInt->getSExtValue();
          IsLoop1BoundConstant = true;
        } else {
          Variable1 = VariablesMap[Instr.getOperand(1)];
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
        else {
          Variable2 = VariablesMap[Instr.getOperand(1)];
        }
      }
    }

    int Loop1StartValue = -1;
    bool IsLoop1StartValueConstant = false;
    Value *StartVariable1;
    BasicBlock *PreLoop1 = L1->getLoopPredecessor();
    if (PreLoop1) {
      for(BasicBlock::iterator I = PreLoop1->begin(), E = PreLoop1->end(); I!=E; ++I){
        Instruction *Instr = &*I;
        if(isa<StoreInst>(Instr)) {
          if(ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr->getOperand(0))) {
            Loop1StartValue = ConstInt->getSExtValue(); // Last store value will always be the loop counter start value.
            IsLoop1StartValueConstant = true;
          } else {
            StartVariable1 = VariablesMap[Instr->getOperand(0)];
            IsLoop1StartValueConstant = false;
          }
        }
      }
    } else {
      return false;
    }

    int Loop2StartValue = -1;
    bool IsLoop2StartValueConstant = false;
    Value *StartVariable2;
    BasicBlock *PreLoop2 = L2->getLoopPredecessor();
    if (PreLoop2) {
      for(BasicBlock::iterator I = PreLoop2->begin(), E = PreLoop2->end(); I!=E; ++I){
        Instruction *Instr = &*I;
        if(isa<StoreInst>(Instr)) {
          if(ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr->getOperand(0))) {
            Loop2StartValue = ConstInt->getSExtValue(); // Last store value will always be the loop counter start value.
            IsLoop2StartValueConstant = true;
          } else {
            StartVariable2 = VariablesMap[Instr->getOperand(0)];
            IsLoop2StartValueConstant = false;
          }
        }
      }
    } else {
      return false;
    }
    if(IsLoop1BoundConstant && IsLoop2BoundConstant) {
      if(IsLoop1StartValueConstant && IsLoop2StartValueConstant) {
        return Loop1Bound == Loop2Bound && Loop1StartValue == Loop2StartValue;
      }
      else if (!IsLoop1StartValueConstant && !IsLoop2StartValueConstant) {
        return Loop1Bound == Loop2Bound && StartVariable1 == StartVariable2;
      } else {
        return false;
      }
    }
    else if (!IsLoop1BoundConstant && !IsLoop2BoundConstant) {
      if(IsLoop1StartValueConstant && IsLoop2StartValueConstant) {
        return Variable1 == Variable2 && Loop1StartValue == Loop2StartValue;
      }
      else if (!IsLoop1StartValueConstant && !IsLoop2StartValueConstant) {
        return Variable1 == Variable2 && StartVariable1 == StartVariable2;
      } else {
        return false;
      }
    }

    return false;
  }

  /// Do all checks to figure out if loops can be fused.
  bool CanFuseLoops(FusionCandidate *L1, FusionCandidate *L2,
                    ScalarEvolution &SE) {
    return HaveSameTripCounts(L1->getLoop(), L2->getLoop());
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

    MapVariables(&F);

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

      dbgs() << "HAVE SAME TRIP COUNTS: " << HaveSameTripCounts(FunctionLoops[0], FunctionLoops[1]) << '\n';

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
