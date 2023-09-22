#ifndef LIB_FUSIONCANDIDATE_H
#define LIB_FUSIONCANDIDATE_H

#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

/// Class used to represent a fusion candidate.
class FusionCandidate {
public:

  FusionCandidate(Loop *L) : L(L) {
    Preheader = L->getLoopPreheader();
    Header = L->getHeader();
    ExitingBlock = L->getExitingBlock();
    Latch = L->getLoopLatch();
    this->setLoopVariables();
  };


  /// Checks if a loop is a candidate for a loop fusion.
  auto isCandidateForFusion() const -> bool;
  auto loop() const -> Loop*;

  Loop *getLoop();
  inline BasicBlock *getPreheader() { return Preheader; };
  inline BasicBlock *getHeader() { return Header; };
  inline BasicBlock *getExitingBlock() { return ExitingBlock; };
  inline BasicBlock *getLatch() { return Latch; };
  void setLoopVariables();
  std::vector<Value *> getLoopVariables();


private:

  auto hasSingleEntryPoint() const -> bool;
  auto hasSingleExitPoint() const -> bool;
  
  // Loop that represents a fusion candidate
  Loop *L;

  std::vector<Value *> LoopVariables;
  BasicBlock *Preheader;
  BasicBlock *Header;
  BasicBlock *ExitingBlock;
  BasicBlock *Latch;
};

#endif // LIB_FUSIONCANDIDATE_H
