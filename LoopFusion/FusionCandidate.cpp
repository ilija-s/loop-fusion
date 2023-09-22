//
// Created by ilijastc on 7/17/23.
//

#include "FusionCandidate.h"

auto FusionCandidate::isCandidateForFusion() const -> bool { return true; }

Loop *FusionCandidate::getLoop() {
    return L;
}

void FusionCandidate::setLoopVariables() {
    BasicBlock *Header = L->getHeader();
    for(BasicBlock *BB : L->getBlocks()) {
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