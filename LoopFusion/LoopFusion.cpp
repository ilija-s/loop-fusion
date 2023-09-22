#include "FusionCandidate.h"
#include "assert.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/CodeMoverUtils.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"

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

  bool AreLoopsAdjacent(Loop *L1, Loop *L2) {

    // At this point we know that L1 and L2 are both candidates
    // This means that L1 has one exit block and L2 has one entering block
    // The only thing is to check if the exit block of L1 is the same as the
    // entry block of L2

    BasicBlock *L1ExitBlock = L1->getExitBlock();
    BasicBlock *L2Preheader = L2->getLoopPreheader();

    return L1ExitBlock == L2Preheader;
  }

  void MapVariables(Function *F) {
    for (BasicBlock &BB : *F) {
      for (Instruction &Instr : BB) {
        if (isa<LoadInst>(&Instr)) {
          VariablesMap[&Instr] = Instr.getOperand(0);
        }
      }
    }
  }

  bool HaveSameBound(Loop *L1, Loop *L2) {
    BasicBlock *Loop1Header = L1->getHeader();

    Value *Variable1;
    Value *Variable2;

    int Loop1Bound = -1;
    bool IsLoop1BoundConstant = false;

    for (Instruction &Instr : *Loop1Header) {
      if (isa<ICmpInst>(&Instr)) {
        if (ConstantInt *ConstInt =
                dyn_cast<ConstantInt>(Instr.getOperand(1))) {
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
        if (ConstantInt *ConstInt =
                dyn_cast<ConstantInt>(Instr.getOperand(1))) {
          Loop2Bound = ConstInt->getSExtValue();
          IsLoop2BoundConstant = true;
        } else {
          Variable2 = VariablesMap[Instr.getOperand(1)];
        }
      }
    }

    if (IsLoop1BoundConstant && IsLoop2BoundConstant) {
      return Loop1Bound == Loop2Bound;
    } else if (!IsLoop1BoundConstant && !IsLoop2BoundConstant) {
      return Variable1 == Variable2;
    }
    return false;
  }

  bool HaveSameStartValue(Loop *L1, Loop *L2) {
    int Loop1StartValue = -1;
    bool IsLoop1StartValueConstant = false;
    Value *StartVariable1;
    BasicBlock *PreLoop1 = L1->getLoopPredecessor();
    if (PreLoop1) {
      for (BasicBlock::iterator I = PreLoop1->begin(), E = PreLoop1->end();
           I != E; ++I) {
        Instruction *Instr = &*I;
        if (isa<StoreInst>(Instr)) {
          if (ConstantInt *ConstInt =
                  dyn_cast<ConstantInt>(Instr->getOperand(0))) {
            Loop1StartValue =
                ConstInt->getSExtValue(); // Last store value will always be the
                                          // loop counter start value.
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
      for (BasicBlock::iterator I = PreLoop2->begin(), E = PreLoop2->end();
           I != E; ++I) {
        Instruction *Instr = &*I;
        if (isa<StoreInst>(Instr)) {
          if (ConstantInt *ConstInt =
                  dyn_cast<ConstantInt>(Instr->getOperand(0))) {
            Loop2StartValue =
                ConstInt->getSExtValue(); // Last store value will always be the
                                          // loop counter start value.
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

    if (IsLoop1StartValueConstant && IsLoop2StartValueConstant) {
      return Loop1StartValue == Loop2StartValue;
    } else if (!IsLoop1StartValueConstant && !IsLoop2StartValueConstant) {
      return StartVariable1 == StartVariable2;
    }
    return false;
  }

  bool HaveSameLatchValue(Loop *L1, Loop *L2) {
    BinaryOperator *BinOp1;
    int Loop1LatchValue = -1;
    bool IsLoop1LatchConstant = false;
    Value *Latch1Variable;
    BasicBlock *Loop1Latch = L1->getLoopLatch();
    if (Loop1Latch) {
      for (BasicBlock::iterator I = Loop1Latch->begin(), E = Loop1Latch->end();
           I != E; ++I) {
        Instruction *Instr = &*I;
        if (isa<BinaryOperator>(Instr)) {
          BinOp1 = cast<BinaryOperator>(Instr);
          if (ConstantInt *ConstInt =
                  dyn_cast<ConstantInt>(Instr->getOperand(1))) {
            Loop1LatchValue = ConstInt->getSExtValue();
            IsLoop1LatchConstant = true;
          } else {
            Latch1Variable = VariablesMap[Instr->getOperand(1)];
            IsLoop1LatchConstant = false;
          }
        }
      }
    } else {
      return false;
    }

    BinaryOperator *BinOp2;
    int Loop2LatchValue = -1;
    bool IsLoop2LatchConstant = false;
    Value *Latch2Variable;
    BasicBlock *Loop2Latch = L2->getLoopLatch();
    if (Loop2Latch) {
      for (BasicBlock::iterator I = Loop2Latch->begin(), E = Loop2Latch->end();
           I != E; ++I) {
        Instruction *Instr = &*I;
        if (isa<BinaryOperator>(Instr)) {
          BinOp2 = cast<BinaryOperator>(Instr);
          if (ConstantInt *ConstInt =
                  dyn_cast<ConstantInt>(Instr->getOperand(1))) {
            Loop2LatchValue = ConstInt->getSExtValue();
            IsLoop2LatchConstant = true;
          } else {
            Latch2Variable = VariablesMap[Instr->getOperand(1)];
            IsLoop2LatchConstant = false;
          }
        }
      }
    } else {
      return false;
    }

    if (BinOp1->getOpcode() != BinOp2->getOpcode()) {
      return false;
    }

    if (IsLoop1LatchConstant && IsLoop2LatchConstant) {
      return Loop1LatchValue == Loop2LatchValue;
    } else if (!IsLoop1LatchConstant && !IsLoop2LatchConstant) {
      return Latch1Variable == Latch2Variable;
    }
    return false;
  }

  bool HaveSameTripCounts(Loop *L1, Loop *L2) {
    return HaveSameBound(L1, L2) && HaveSameStartValue(L1, L2) &&
           HaveSameLatchValue(L1, L2);
  }

  /// Do all checks to figure out if loops can be fused.
  bool CanFuseLoops(FusionCandidate *L1, FusionCandidate *L2,
                    ScalarEvolution &SE) {
    return HaveSameTripCounts(L1->getLoop(), L2->getLoop()) &&
           AreLoopsAdjacent(L1->getLoop(), L2->getLoop());
  }

  /// Function that will fuse loops based on previously established candidates.
  void FuseLoops(FusionCandidate *L1, FusionCandidate *L2, Function &F,
                 LoopInfo &LI, DominatorTree &DT, PostDominatorTree &PDT,
                 DependenceInfo &DI, ScalarEvolution &SE) {

    dbgs() << "Moving instructions from Loop 2 preheader to Loop 1 preheader\n";
    moveInstructionsToTheEnd(*L2->getPreheader(), *L1->getPreheader(), DT, PDT,
                             DI);

    dbgs() << "Redirecting basic blocks...\n";
    // Replace all uses of Loop2 Preheader with Loop2 Header
    L1->getExitingBlock()->getTerminator()->replaceUsesOfWith(
        L2->getPreheader(), L2->getHeader());

    // Preheader of Loop2 is not used anymore
    L2->getPreheader()->getTerminator()->eraseFromParent();
    new UnreachableInst(L2->getPreheader()->getContext(), L2->getPreheader());

    // Loop1 latch now jumps to the Loop2 header and Loop2 latch jumps to the
    // Loop1 header
    L1->getLatch()->getTerminator()->replaceUsesOfWith(L1->getHeader(),
                                                       L2->getHeader());
    L2->getLatch()->getTerminator()->replaceUsesOfWith(L2->getHeader(),
                                                       L1->getHeader());
    dbgs() << "Loop basic blocks redirected.\n";

    // Removing Loop2 Preheader since it is empty now
    LI.removeBlock(L2->getPreheader());

    SE.forgetLoop(L2->getLoop());
    SE.forgetLoop(L1->getLoop());
    SE.forgetLoopDispositions();

    // Recalculating Dominator and Post-Dominator Trees
    DT.recalculate(F);
    PDT.recalculate(F);

    // Modify the loop body to incorporate the instructions from both loops.
    // We need to ensure that the instructions are ordered correctly to
    // maintain correct program semantics.

    // Move instructions from L1 Latch to L2 Latch.
    moveInstructionsToTheBeginning(*L1->getLatch(), *L2->getLatch(), DT, PDT,
                                   DI);
    MergeBlockIntoPredecessor(L1->getLatch()->getUniqueSuccessor(), nullptr,
                              &LI);

    // Recalculating Dominator and Post-Dominator Trees
    DT.recalculate(F);
    PDT.recalculate(F);

    // Merging all Loop2 blocks with Loop1 blocks
    SmallVector<BasicBlock *> Blocks(L2->getLoop()->blocks());
    for (BasicBlock *BB : Blocks) {
      L1->getLoop()->addBlockEntry(BB);
      L2->getLoop()->removeBlockFromLoop(BB);

      if (LI.getLoopFor(BB) != L2->getLoop())
        continue;
      LI.changeLoopFor(BB, L1->getLoop());
    }

    dbgs() << "Printing Loop1 blocks.\n";
    for (auto &Block : L1->getLoop()->blocks()) {
      dbgs() << *Block;
    }

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

      dbgs() << "HAVE SAME TRIP COUNTS: "
             << HaveSameTripCounts(FunctionLoops[0], FunctionLoops[1]) << '\n';

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
