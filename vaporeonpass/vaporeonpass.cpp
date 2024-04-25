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

  DenseSet<Value *> TaintedPointers;
  DenseSet<Value *> MWTaintedPointers;

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
    TaintedPointers.insert(V);
    // if (TaintedPointers.insert(V).second) {
    //   for (User *U : V->users()) {
    //     if (Instruction *Inst = dyn_cast<Instruction>(U)) {
    //       if (CallInst *CI = dyn_cast<CallInst>(Inst)) {
    //         // propagate taint to return value or global variables
    //         if (CI->getCalledFunction() == nullptr ||
    //             CI->getCalledFunction()->isDeclaration()) {
    //           markTainted(Inst);
    //         }
    //       }
    //     }
    //   }
    // }
  }

  void propagateTaintedPointers() {
    // Propagation of tainted pointer status
    std::deque<Value *> propagation(std::begin(TaintedPointers),
                                    std::end(TaintedPointers));
    dbgs() << "[STAGE 2: Tainted Pointer Propagation]\n";
    while (!propagation.empty()) {
      auto taint = propagation.front();
      propagation.pop_front();
      dbgs() << "Found tainted pointer " << *taint << "\n";
      for (auto use : taint->users()) {
        if (TaintedPointers.contains(use))
          continue;
        dbgs() << "  Marking " << *use << " as tainted because it uses "
               << *taint << "\n";
        markTainted(use);
        propagation.push_back(use);
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

    propagateTaintedPointers();
  }

  bool isMemoryWrite(Value *V, Instruction &I) {
    if (auto *SI = dyn_cast<StoreInst>(&I)) {
      return SI->getPointerOperand() == V;
    }
    if (auto *CI = dyn_cast<CallInst>(&I)) {
      if (CI->getCalledFunction()) {
        unsigned numOperands = CI->getNumOperands() - 1;
        for (unsigned i = 0; i < numOperands; ++i) {
          return CI->getArgOperand(i) == V;
        }
      }
    }
    // are there other memory write instructions?

    // else if (auto *MI = dyn_cast<MemIntrinsic>(&I)) {
    //   return MI->getRawDest() == V;
    // }
    return false;
  }

  void markMemoryTainted(Value *V) { MWTaintedPointers.insert(V); }

  void identifyMemoryWrites(Function &F) {
    for (Instruction &I : instructions(F)) {
      for (Value *V : TaintedPointers) {
        if (isMemoryWrite(V, I)) {
          markMemoryTainted(V);
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

  void printMWTaintedPointers(Function &F) {
    for (Instruction &I : instructions(F)) {
      if (MWTaintedPointers.count(&I)) {
        dbgs() << "MWTainted: " << I << "\n";
      }
    }
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    identifyTaintedPointers(F);
    if (PRINTDEBUG) {
      dbgs() << "BEFORE MEMORY WRITER\n";
      printTaintedPointers(F);
    }
    identifyMemoryWrites(F);
    if (PRINTDEBUG) {
      dbgs() << "AFTER MEMORY WRITER\n";
      printMWTaintedPointers(F);
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
