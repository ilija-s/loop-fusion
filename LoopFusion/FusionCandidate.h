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
    ExitBlock = L->getExitBlock();
    this->setLoopVariables();
  };

  /// Checks if a loop is a candidate for a loop fusion.
  auto isCandidateForFusion() const -> bool;

  inline auto getLoop() const -> Loop * { return L; };
  inline auto getPreheader() const -> BasicBlock * { return Preheader; };
  inline auto getHeader() const -> BasicBlock * { return Header; };
  inline auto getExitingBlock() const -> BasicBlock * { return ExitingBlock; };
  inline auto getExitBlock() const -> BasicBlock * { return ExitBlock; };
  inline auto getLatch() const -> BasicBlock * { return Latch; };

  void setLoopVariables();
  std::vector<Value *> getWriteVariables();
  std::vector<Value *> getReadVariables();

private:
  auto hasSingleEntryPoint() const -> bool;
  auto hasSingleExitPoint() const -> bool;

  // Loop that represents a fusion candidate
  Loop *L;

  std::vector<Value *> WriteVariables;
  std::vector<Value *> ReadVariables;
  BasicBlock *Preheader;
  BasicBlock *Header;
  BasicBlock *ExitingBlock;
  BasicBlock *ExitBlock;
  BasicBlock *Latch;
};

#endif // LIB_FUSIONCANDIDATE_H
