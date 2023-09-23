#include "FusionCandidate.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "loop-fusion"

auto FusionCandidate::isCandidateForFusion() const -> bool {
  for (auto &BB : L->getBlocks()) {
    for (auto &Inst : *BB) {
      if (Inst.mayThrow()) {
        dbgs() << "Loop contains instruction that may throw exception.\n";
        return false;
      }
      if (StoreInst *Store = dyn_cast<StoreInst>(&Inst)) {
        if (Store->isVolatile()) {
          dbgs() << "Loop contains volatile memory access.\n";
          return false;
        }
      }
      if (LoadInst *Load = dyn_cast<LoadInst>(&Inst)) {
        if (Load->isVolatile()) {
          dbgs() << "Loop contains volatile memory access.\n";
          return false;
        }
      }
    }
  }

  if (!L->isLoopSimplifyForm()) {
    dbgs() << "Loop is not in simplified form.\n";
    return false;
  }

  if (!L->getLoopPreheader() || !L->getHeader() || !L->getExitingBlock() ||
      !L->getLoopLatch()) {
    dbgs() << "Necessary loop information is not available(preheader, header, "
              "latch, exiting block).\n";
    return false;
  }

  if (!hasSingleEntryPoint() || !hasSingleExitPoint()) {
    dbgs() << "Loop does not have single entry or exit point.\n";
    return false;
  }

  if (L->isAnnotatedParallel()) {
    dbgs() << "Loop is annotated parallel.\n";
    return false;
  }

  return true;
}

auto FusionCandidate::hasSingleEntryPoint() const -> bool {

  BasicBlock *LoopPredecessor = L->getLoopPredecessor();

  return LoopPredecessor != nullptr;
}

auto FusionCandidate::hasSingleExitPoint() const -> bool {

  SmallVector<BasicBlock *> ExitBlocks{};
  L->getExitBlocks(ExitBlocks);

  return ExitBlocks.size() == 1;
}

void FusionCandidate::setLoopVariables() {
  BasicBlock *Header = L->getHeader();
  for (BasicBlock *BB : L->getBlocks()) {
    if (BB != Header && !L->isLoopLatch(BB) && !L->isLoopExiting(BB)) {
      for (Instruction &Instr : *BB) {
        if (isa<LoadInst>(&Instr)) {
          LoopVariables.push_back(Instr.getOperand(0));
        }
        if (isa<StoreInst>(&Instr)) {
          LoopVariables.push_back(Instr.getOperand(1));
        }
        if (isa<GetElementPtrInst>(&Instr)) {
          LoopVariables.push_back(Instr.getOperand(0));
        }
      }
    }
  }
}

std::vector<Value *> FusionCandidate::getLoopVariables() {
  return LoopVariables;
}