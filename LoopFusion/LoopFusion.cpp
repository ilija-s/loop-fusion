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

  bool areLoopsAdjacent(Loop *L1, Loop *L2) {
    // At this point we know that L1 and L2 are both candidates
    // This means that L1 has one exit block and L2 has one entering block
    // The only thing is to check if the exit block of L1 is the same as the
    // entry block of L2

    BasicBlock *L1ExitBlock = L1->getExitBlock();
    BasicBlock *L2Preheader = L2->getLoopPreheader();

    return L1ExitBlock == L2Preheader;
  }

  void mapVariables(Function *F) {
    for (BasicBlock &BB : *F) {
      for (Instruction &Instr : BB) {
        if (isa<LoadInst>(&Instr)) {
          VariablesMap[&Instr] = Instr.getOperand(0);
        }
      }
    }
  }

  bool haveSameBound(Loop *L1, Loop *L2) {
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
    }
    if (!IsLoop1BoundConstant && !IsLoop2BoundConstant) {
      return Variable1 == Variable2;
    }
    return false;
  }

  bool haveSameStartValue(Loop *L1, Loop *L2) {
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
    }
    if (!IsLoop1StartValueConstant && !IsLoop2StartValueConstant) {
      return StartVariable1 == StartVariable2;
    }
    return false;
  }

  bool haveSameLatchValue(Loop *L1, Loop *L2) {
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
    }
    if (!IsLoop1LatchConstant && !IsLoop2LatchConstant) {
      return Latch1Variable == Latch2Variable;
    }
    return false;
  }

  bool changesCounter(Loop *L) {
    BasicBlock *Header = L->getHeader();
    Value *Counter;
    for (Instruction &Instr : *Header) {
      if(isa<LoadInst>(&Instr)) {
        Counter = Instr.getOperand(0);
        break;
      }
    }
    for (BasicBlock *BB : L->getBlocks()) {
      if (BB != Header && !L->isLoopLatch(BB) && !L->isLoopExiting(BB)) {
        for (Instruction &Instr : *BB) {
          if (isa<StoreInst>(&Instr)) {
            if (Instr.getOperand(1) == Counter) {
              return true;
            }
          }
        }
      }
    }
    return false;
  }

  bool haveSameTripCounts(Loop *L1, Loop *L2) {
    return haveSameBound(L1, L2) && haveSameStartValue(L1, L2) &&
           haveSameLatchValue(L1, L2) && !changesCounter(L1) && !changesCounter(L2);
  }

  bool areDependent(FusionCandidate *F1, FusionCandidate *F2) {
    std::vector<Value *> ReadVariables1 = F1->getReadVariables();
    std::vector<Value *> ReadVariables2 = F2->getReadVariables();
    std::vector<Value *> WriteVariables1 = F1->getWriteVariables();
    std::vector<Value *> WriteVariables2 = F2->getWriteVariables();

    for (Value *V1 : WriteVariables1) {
      for (Value *V2 : ReadVariables2) {
        // dbgs() << "VALUE 1: " << V1 << ", VALUE 2: "  << V2 << '\n';
        if (V1 == V2) {
          return true;
        }
      }
      for (Value *V2 : WriteVariables2) {
        // dbgs() << "VALUE 1: " << V1 << ", VALUE 2: "  << V2 << '\n';
        if (V1 == V2) {
          return true;
        }
      }
    }
    for (Value *V1 : WriteVariables2) {
      for (Value *V2 : ReadVariables1) {
        if (V1 == V2) {
          return true;
        }
      }
    }
    return false;
  }

  /// Do all checks to figure out if loops can be fused.
  bool canFuseLoops(FusionCandidate *L1, FusionCandidate *L2,
                    ScalarEvolution &SE) {
    return haveSameTripCounts(L1->getLoop(), L2->getLoop()) &&
           !areDependent(L1, L2) &&
           areLoopsAdjacent(L1->getLoop(), L2->getLoop());
  }

  /// Function that will fuse loops based on previously established candidates.
  void FuseLoops(FusionCandidate *L1, FusionCandidate *L2, Function &F,
                 LoopInfo &LI, DominatorTree &DT, PostDominatorTree &PDT,
                 DependenceInfo &DI, ScalarEvolution &SE) {

    // Moving instructions from Loop 2 Preheader to Loop 1 Preheader
    moveInstructionsToTheEnd(*L2->getPreheader(), *L1->getPreheader(), DT, PDT,
                             DI);

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

    // Removing Loop2 Preheader since it is empty now
    LI.removeBlock(L2->getPreheader());

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

      // If BB is not a part of Loop2 that means that it was successfully moved
      // otherwise we need to assign BB to Loop1 inside-of LoopInfo
      if (LI.getLoopFor(BB) != L2->getLoop())
        continue;
      LI.changeLoopFor(BB, L1->getLoop());
    }

    // Remove the Loop2 from the LLVM IR
    EliminateUnreachableBlocks(F);
    LI.erase(L2->getLoop());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequiredID(LoopSimplifyID);
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<DependenceAnalysisWrapperPass>();
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
    auto &DI = getAnalysis<DependenceAnalysisWrapperPass>().getDI();
    auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    auto &PDT = getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

    mapVariables(&F);

    // Collect fusion candidates.
    SmallVector<Loop *> FunctionLoops(LI.begin(), LI.end());
    for (const auto &L : FunctionLoops) {
      FusionCandidate FC(L);
      if (FC.isCandidateForFusion()) {
        FusionCandidates.emplace_back(FC);
      }
    }

    if (FusionCandidates.size() < 2) {
      dbgs() << "Not enough candidates for fusion.\n";
      return true;
    }

    std::reverse(std::begin(FusionCandidates), std::end(FusionCandidates));
    for (int I = 0; I < FusionCandidates.size() - 1; ++I) {
      dbgs() << "HAVE SAME TRIP COUNTS: "
             << haveSameTripCounts(FusionCandidates[I].getLoop(),
                                   FusionCandidates[I + 1].getLoop())
             << '\n';

      dbgs() << "ARE ADJECENT: "
             << areLoopsAdjacent(FusionCandidates[I].getLoop(),
                                 FusionCandidates[I + 1].getLoop())
             << '\n';

      dbgs() << "ARE NOT DEPENDANT: "
             << !areDependent(&FusionCandidates[I], &FusionCandidates[I + 1])
             << '\n';
      dbgs() << "CAN FUSE: "
             << canFuseLoops(&FusionCandidates[I], &FusionCandidates[I + 1], SE)
             << '\n';
      if (canFuseLoops(&FusionCandidates[I], &FusionCandidates[I + 1], SE)) {
        FuseLoops(&FusionCandidates[I], &FusionCandidates[I + 1], F, LI, DT,
                  PDT, DI, SE);
      }
    }

    return true;
  }
};
} // namespace

char LoopFusion::ID = 0;
static RegisterPass<LoopFusion> X("loopfusion", "Loop Fusion Pass");
