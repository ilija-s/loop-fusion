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
  };

  /// Checks if a loop is a candidate for a loop fusion.
  auto isCandidateForFusion() const -> bool;

  Loop *getLoop();
  inline BasicBlock *getPreheader() { return Preheader; };
  inline BasicBlock *getHeader() { return Header; };
  inline BasicBlock *getExitingBlock() { return ExitingBlock; };
  inline BasicBlock *getLatch() { return Latch; };

private:
  // Loop that represents a fusion candidate
  Loop *L;
  BasicBlock *Preheader;
  BasicBlock *Header;
  BasicBlock *ExitingBlock;
  BasicBlock *Latch;
};

#endif // LIB_FUSIONCANDIDATE_H
