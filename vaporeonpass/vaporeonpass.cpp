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
#include <iostream>

using namespace llvm;

bool PRINTDEBUG = true;

namespace {
struct VaporeonPass : public PassInfoMixin<VaporeonPass> {

  DenseMap<Value *, bool> TaintedPointers;

  bool isExternalSource(const Function *F) {
    static const DenseSet<StringRef> Sources = {"scanf", "fgets", "read"};
    return Sources.contains(F->getName());
  }

  void markTainted(Value *V) {
    if (TaintedPointers.insert({V, true}).second) {
      for (User *U : V->users()) {
        if (Instruction *Inst = dyn_cast<Instruction>(U)) {
          if (CallInst *CI = dyn_cast<CallInst>(Inst)) {
            // propagate taint to return value or global variables
            if (CI->getCalledFunction() == nullptr ||
                CI->getCalledFunction()->isDeclaration()) {
              markTainted(Inst);
            }
          } else {
            markTainted(Inst);
          }
        }
      }
    }
  }

  void identifyTaintedPointers(Function &F) {
    for (Instruction &I : instructions(F)) {
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (CI->getCalledFunction() &&
            isExternalSource(CI->getCalledFunction())) {
          unsigned numOperands = CI->getNumOperands() - 1;
          for (unsigned i = 0; i < numOperands; ++i) {
            Value *argVal = CI->getArgOperand(i);
            if (argVal) {
              markTainted(argVal);
            }
          }
        }
      }
    }
  }

  void printTaintedPointers(Function &F) {
    for (Instruction &I : instructions(F)) {
      if (TaintedPointers.count(&I)) {
        dbgs() << "Tainted: " << I << "\n";
      }
    }
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    identifyTaintedPointers(F);
    if (PRINTDEBUG) {
      printTaintedPointers(F);
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
