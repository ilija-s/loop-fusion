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

private:
  // Loop that represents a fusion candidate
  Loop *L;
};

#endif // LIB_FUSIONCANDIDATE_H
