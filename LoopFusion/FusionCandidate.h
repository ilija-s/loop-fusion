#ifndef LIB_FUSIONCANDIDATE_H
#define LIB_FUSIONCANDIDATE_H

#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

/// Class used to represent a fusion candidate.
class FusionCandidate {
public:
  FusionCandidate(Loop *L) : L(L){};

  /// Checks if a loop is a candidate for a loop fusion.
  auto isCandidateForFusion() const -> bool;
  auto loop() const -> Loop*;

private:

  auto hasSingleEntryPoint() const -> bool;
  auto hasSingleExitPoint() const -> bool;
  
  // Loop that represents a fusion candidate
  Loop *L;
};

#endif // LIB_FUSIONCANDIDATE_H
