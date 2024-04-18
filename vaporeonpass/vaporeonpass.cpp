#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

using namespace llvm;

namespace {
struct VaporeonPass : public PassInfoMixin<VaporeonPass> {

  DenseMap<Value *, bool, DenseMapInfo<Value *>> TaintedPointers;

  bool isExternalSource(CallInst *CI) {
    // TODO: Generalize this check
    StringRef FuncName = CI->getCalledFunction()->getName();
    return FuncName == "scanf" || FuncName.start_with_insensitive("read");
  }

  void markTainted(Value *V) {
    if (!TaintedPointers.count(V)) {
      TaintedPointers[V] = true;
      for (User *U : V->users()) {
        if (auto *I = dyn_cast<Instruction>(U)) {
          markTainted(I);
        }
      }
    }
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    for (Instruction &I : instructions(F)) {
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (isExternalSource(CI)) {
          if (Value *RetVal = CI) {
            markTainted(RetVal);
          }
        }
      }
    }
    return PreservedAnalyses::none();
  }
};
} // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "VaporeonPass", "v0.1", [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "vaporeonpass") {
                    FPM.addPass(VaporeonPass());
                    return true;
                  }
                  return false;
                });
          }};
}