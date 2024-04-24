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
#include "llvm/IR/Instruction.h"
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

  DenseSet<Value *> TaintedPointers;

  // Multipass 
  // Set of all tainted functions 
  // populate this beforehand 
  // then do a second pass and look for specifically ret

  // https://stackoverflow.com/questions/26558197/unsafe-c-functions-and-the-replacement
  bool isExternalSource(const Function *F) {
    static const DenseSet<StringRef> Sources = {
        "scanf",    "fgets",   "read",     "gets",     "sprintf",
        "vsprintf", "strncat", "copy_buf", "makepath", "_splitpath",
        "sscanf",   "strlen",  "strncpy",  "strcpy",   "read_chunk"};
    return Sources.contains(F->getName());

    // TODO: We should also check for user input from files, network, etc.
  }

  void markTainted(Value *V) {
    if (TaintedPointers.insert(V).second) {
      for (User *U : V->users()) {
        auto *FunctionUserBelongsTo = dyn_cast<Instruction>(U)->getFunction();
        if (FunctionUserBelongsTo) {
          if (PRINTDEBUG) {
            dbgs() << "Marking " << *U << " as tainted because it uses " << *V
                   << "\n";
          }
          auto *FunctionOutput = FunctionUserBelongsTo->getReturnType();
          TaintedPointers.insert(FunctionOutput);
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

    // Propagation of tainted pointer status
    std::deque<Value *> propagation(std::begin(TaintedPointers),
                                    std::end(TaintedPointers));
    dbgs() << "[STAGE 2: Tainted Pointer Propagation]\n";
    while (!propagation.empty()) {
      auto taint = propagation.front();
      propagation.pop_front();
      dbgs() << "Found tainted pointer " << *taint << "\n";
      for (auto use : taint->users()) {
        dbgs() << "  Found a user " << *use << "\n";
        if (TaintedPointers.contains(use))
          continue;
        dbgs() << "  Marking " << *use << " as tainted because it uses "
               << *taint << "\n";
        markTainted(use);
        propagation.push_back(use);
      }
    }
  }

  void printTaintedPointers(Function &F) {
    for (Instruction &I : instructions(F)) {
      if (TaintedPointers.contains(&I)) {
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
