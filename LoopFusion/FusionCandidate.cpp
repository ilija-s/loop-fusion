//
// Created by ilijastc on 7/17/23.
//

#include "FusionCandidate.h"
#include "llvm/IR/BasicBlock.h"

auto FusionCandidate::isCandidateForFusion() const -> bool { 
    return hasSingleEntryPoint() && hasSingleExitPoint(); 
}

auto FusionCandidate::loop() const -> Loop* {
    return L;
}

auto FusionCandidate::hasSingleEntryPoint() const -> bool {
    
    BasicBlock* LoopPredecessor = L->getLoopPredecessor();

    return LoopPredecessor != nullptr;
}

auto FusionCandidate::hasSingleExitPoint() const -> bool {

    SmallVector<BasicBlock*> ExitBlocks{};
    L->getExitBlocks(ExitBlocks);

    return ExitBlocks.size() == 1;
}   

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