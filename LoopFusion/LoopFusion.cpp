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
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils.h"
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
      for(BasicBlock::iterator I = Loop1Latch->begin(), E = Loop1Latch->end(); I != E; ++I) {
        Instruction *Instr = &*I;
        if (isa<BinaryOperator>(Instr)) {
          BinOp1 = cast<BinaryOperator>(Instr);
          if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr->getOperand(1))) {
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
      for(BasicBlock::iterator I = Loop2Latch->begin(), E = Loop2Latch->end(); I != E; ++I) {
        Instruction *Instr = &*I;
        if (isa<BinaryOperator>(Instr)) {
          BinOp2 = cast<BinaryOperator>(Instr);
          if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr->getOperand(1))) {
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

  bool haveSameTripCounts(Loop *L1, Loop *L2) {
    return haveSameBound(L1, L2) && haveSameStartValue(L1, L2) &&
           haveSameLatchValue(L1, L2);
  }

  bool areDependent(FusionCandidate *F1, FusionCandidate *F2) {
    std::vector<Value *> Variables1 = F1->getLoopVariables();
    std::vector<Value *> Variables2 = F2->getLoopVariables();

    for(Value *V1 : Variables1) {
      for(Value *V2 : Variables2) {
        //dbgs() << "VALUE 1: " << V1 << ", VALUE 2: "  << V2 << '\n';
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
    return haveSameTripCounts(L1->getLoop(), L2->getLoop()) && !areDependent(L1, L2);
  }

  /// Function that will fuse loops based on previously established candidates.
  void fuseLoops(FusionCandidate *L1, FusionCandidate *L2) {
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

    mapVariables(&F);

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
             << haveSameTripCounts(FunctionLoops[0], FunctionLoops[1]) << '\n';

      if (!L->isLoopSimplifyForm()) {
        dbgs() << "Loop " << L->getName() << " is not in simplified form\n";
      }
      FusionCandidate FC(L);
      if (FC.isCandidateForFusion()) {
        FusionCandidates.emplace_back(FC);
      }
    }

    dbgs() << "ARE DEPENDANT: "
           << areDependent(&FusionCandidates[0], &FusionCandidates[1]) << '\n';

    dbgs() << "CAN FUSE: "
           << canFuseLoops(&FusionCandidates[0], &FusionCandidates[1], SE) << '\n';

    if (canFuseLoops(&FusionCandidates[0], &FusionCandidates[1], SE)) {
      fuseLoops(&FusionCandidates[0], &FusionCandidates[1]);
    }

    return true;
  }
};
} // namespace

char LoopFusion::ID = 0;
static RegisterPass<LoopFusion> X("loopfusion", "Loop Fusion Pass");
