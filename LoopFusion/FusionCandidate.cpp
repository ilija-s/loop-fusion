//
// Created by ilijastc on 7/17/23.
//

#include "FusionCandidate.h"

auto FusionCandidate::isCandidateForFusion() const -> bool { return true; }

Loop *FusionCandidate::getLoop() {
    return L;
}
