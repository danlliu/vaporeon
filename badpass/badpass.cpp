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

constexpr bool PRINTDEBUG = true;

namespace {
struct VaporeonPass : public PassInfoMixin<VaporeonPass> {

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    std::vector<StoreInst *> stores;
    struct FatPointer {
      Value *lower, *size;
    };
    DenseMap<Value *, FatPointer> bounds;
    DenseMap<Value *, FatPointer> localVariableBounds;
    std::deque<Instruction *> bfs;

    Type *index_type = llvm::Type::getInt32Ty(F.getContext());
    Type *size_type = llvm::Type::getInt64Ty(F.getContext());
    Instruction *alloca_insertion_point = nullptr;

    int instructionsAdded = 0;

    {
      if (PRINTDEBUG)
        dbgs() << "Starting VAPOREON pass\n";
      std::vector<AllocaInst *> to_fixup;

      // Step 0: incoming parameters

      auto insertionPoint = &*F.getEntryBlock().getFirstNonPHIOrDbgOrAlloca();
      if (PRINTDEBUG)
        dbgs() << "[Step 0] fix up parameters\n";
      for (auto &param : F.args()) {
        if (param.getType()->isPointerTy()) {
          Type *ptr_type = param.getType();
          if (PRINTDEBUG)
            dbgs() << "ptr_type = " << *ptr_type << "\n";
          if (PRINTDEBUG)
            dbgs() << "size_type = " << *size_type << "\n";
          auto fatpointer_type = StructType::create(F.getContext(), "fatptr_t");
          fatpointer_type->setBody({ptr_type, ptr_type, size_type}, false);

          std::vector<User *> users;
          for (auto u : param.users())
            users.push_back(u);

          auto addr = GetElementPtrInst::Create(
              ptr_type, &param,
              {Constant::getIntegerValue(index_type, APInt(32, 1))},
              "unpack_lower", insertionPoint);
          auto lower = new LoadInst(ptr_type, addr, "", insertionPoint);
          if (PRINTDEBUG)
            dbgs() << "lower = " << *lower << "\n";

          auto addr2 = GetElementPtrInst::Create(
              size_type, &param,
              {Constant::getIntegerValue(index_type, APInt(32, 2))},
              "unpack_size", insertionPoint);
          auto size = new LoadInst(size_type, addr2, "", insertionPoint);
          if (PRINTDEBUG)
            dbgs() << "size = " << *size << "\n";

          auto addr3 = GetElementPtrInst::Create(
              ptr_type, &param,
              {Constant::getIntegerValue(index_type, APInt(32, 0))},
              "unpack_ptr", insertionPoint);
          auto raw_pointer = new LoadInst(ptr_type, addr3, "", insertionPoint);

          instructionsAdded += 6;

          for (auto u : users) {
            u->replaceUsesOfWith(&param, raw_pointer);
          }

          if (PRINTDEBUG)
            dbgs() << "raw = " << *raw_pointer << "\n";
          bounds[raw_pointer] = {lower, size};
          bfs.emplace_back(raw_pointer);
        }
      }

      if (PRINTDEBUG)
        dbgs() << "[Step 1] find all allocas\n";
      // Step 1: find all allocas
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto *AI = dyn_cast<AllocaInst>(&I)) {
            if (PRINTDEBUG)
              dbgs() << "Found Alloca " << *AI << "\n";
            auto alloc_type = AI->getAllocatedType();
            alloca_insertion_point = AI->getNextNonDebugInstruction();
            if (alloc_type->isArrayTy()) {
              size_t alloc_size = alloc_type->getArrayNumElements();
              if (PRINTDEBUG)
                dbgs() << "instruction allocates " << alloc_size
                       << " element(s)\n";
              auto InsertionPoint = AI->getNextNonDebugInstruction();
              if (PRINTDEBUG)
                if (!InsertionPoint) {
                  dbgs() << "No insertion point found...\n";
                }

              // Store allocated value into lower
              auto idx = Constant::getIntegerValue(
                  Type::getInt64Ty(F.getContext()), APInt(64, alloc_size));
              bounds[AI] = {AI, idx};
              bfs.emplace_back(AI);
            } else if (alloc_type->isPointerTy()) {
              to_fixup.emplace_back(AI);
            }
          }
        }
      }

      if (PRINTDEBUG)
        dbgs() << "[Step 1] add bounds\n";
      for (auto AI : to_fixup) {
        auto alloc_type = AI->getAllocatedType();
        auto lowerAlloc = new AllocaInst(
            alloc_type, alloc_type->getPointerAddressSpace(), "loc_lower", AI);
        auto sizeAlloc = new AllocaInst(Type::getInt64Ty(F.getContext()),
                                        alloc_type->getPointerAddressSpace(),
                                        "loc_size", AI);
        localVariableBounds[AI] = {lowerAlloc, sizeAlloc};
        if (PRINTDEBUG)
          dbgs() << "Added local variable bounds for " << *AI << "\n";
      }
    }

    DenseSet<Instruction *> visited;
    DenseSet<StoreInst *> ourStores;

    // Step 2: generate code to propagate bounds at runtime
    while (!bfs.empty()) {
      auto front = bfs.front();
      bfs.pop_front();
      if (visited.contains(front))
        continue;
      visited.insert(front);
      if (PRINTDEBUG)
        dbgs() << "front = " << *front << "\n";

      auto [lower, size] = bounds[front];

      for (auto Use : front->users()) {
        if (PRINTDEBUG)
          dbgs() << "found use " << *Use << "\n";
        if (auto Inst = dyn_cast<Instruction>(Use)) {
          // some instruction is using this value, propagate bounds
          if (Inst->getOpcode() == Instruction::PHI) {
            if (bounds.contains(Inst)) {
              // we already found it in a prior pass, now we need to update
              // second argument
              PHINode *lowerPhi = dyn_cast<PHINode>(bounds[Inst].lower);
              PHINode *sizePhi = dyn_cast<PHINode>(bounds[Inst].size);
              lowerPhi->setOperand(1, lower);
              sizePhi->setOperand(1, size);
            } else {
              // first time finding this phi, initialize it
              auto InsertionPoint = front->getNextNonDebugInstruction();
              PHINode *lowerPhi = PHINode::Create(
                  bounds[front].lower->getType(), 2, "lower", InsertionPoint);
              PHINode *sizePhi = PHINode::Create(bounds[front].lower->getType(),
                                                 2, "size", InsertionPoint);
              instructionsAdded += 2;
              lowerPhi->setOperand(0, lower);
              sizePhi->setOperand(0, size);
              bounds[Inst] = {lowerPhi, sizePhi};
              bfs.emplace_back(Inst);
            }
          } else {
            // one potential path, just forward it
            if (auto SI = dyn_cast<StoreInst>(Inst)) {
              if (front == SI->getValueOperand() &&
                  localVariableBounds.contains(SI->getPointerOperand())) {
                // storing FAT pointer into local variable
                auto [loc_lower, loc_size] =
                    localVariableBounds[SI->getPointerOperand()];
                if (PRINTDEBUG)
                  dbgs() << " loc_lower = " << *loc_lower
                         << ", loc_size = " << *loc_size << "\n";
                if (PRINTDEBUG)
                  dbgs() << " lower = " << *lower << ", size = " << *size
                         << "\n";
                ourStores.insert(new StoreInst(lower, loc_lower, SI));
                ourStores.insert(new StoreInst(size, loc_size, SI));
                instructionsAdded += 2;
                for (auto use : SI->getPointerOperand()->users()) {
                  if (PRINTDEBUG)
                    dbgs() << " found use of store " << *use << "\n";
                  if (auto LI = dyn_cast<LoadInst>(use)) {
                    auto [saved_lower, saved_size] =
                        localVariableBounds[LI->getPointerOperand()];
                    auto reload_lower = new LoadInst(LI->getType(), saved_lower,
                                                     "load_lower", LI);
                    auto reload_size =
                        new LoadInst(Type::getInt64Ty(F.getContext()),
                                     saved_size, "load_size", LI);
                    instructionsAdded += 2;
                    bounds[LI] = {reload_lower, reload_size};
                    for (auto load_use : LI->users()) {
                      if (auto inst_use = dyn_cast<Instruction>(load_use)) {
                        bounds[inst_use] = {reload_lower, reload_size};
                        bfs.emplace_back(inst_use);
                      }
                    }
                  }
                }
              } else {
                if (PRINTDEBUG)
                  dbgs() << "how did we get here? " << *SI << "\n";
              }
            } else if (auto CI = dyn_cast<CallInst>(Inst)) {
              for (size_t i = 0; i < CI->arg_size(); ++i) {
                auto param = CI->getArgOperand(i);
                if (auto I = dyn_cast<Instruction>(param)) {
                  if (bounds.contains(I)) {
                    Type *ptr_type = param->getType();
                    auto fatpointer_type =
                        StructType::create(F.getContext(), "fatptr_t");
                    fatpointer_type->setBody({ptr_type, ptr_type, size_type},
                                             false);
                    auto new_param =
                        new AllocaInst(fatpointer_type, F.getAddressSpace(),
                                       "param", alloca_insertion_point);
                    instructionsAdded += 2;
                    {
                      auto addr = GetElementPtrInst::Create(
                          ptr_type, new_param,
                          {Constant::getIntegerValue(index_type, APInt(32, 0))},
                          "pack_ptr", CI);
                      ourStores.insert(new StoreInst(I, addr, CI));
                    }
                    {
                      auto addr = GetElementPtrInst::Create(
                          ptr_type, new_param,
                          {Constant::getIntegerValue(index_type, APInt(32, 1))},
                          "pack_lower", CI);
                      ourStores.insert(
                          new StoreInst(bounds[I].lower, addr, CI));
                    }
                    {
                      auto addr = GetElementPtrInst::Create(
                          size_type, new_param,
                          {Constant::getIntegerValue(index_type, APInt(32, 2))},
                          "pack_size", CI);
                      ourStores.insert(new StoreInst(bounds[I].size, addr, CI));
                    }
                    instructionsAdded += 6;
                    CI->setArgOperand(i, new_param);
                  }
                }
              }
            } else if (!bounds.contains(Inst)) {
              if (PRINTDEBUG)
                dbgs() << "propagating scope for " << *Inst << "\n";
              if (PRINTDEBUG)
                dbgs() << " lower = " << *lower << ", size = " << *size << "\n";
              bounds[Inst] = bounds[front];
              bfs.emplace_back(Inst);
            }
          }
        }
      }
    }

    // Step 3: create trap blockstepbro
    auto *trapBlock =
        BasicBlock::Create(F.getContext(), "helpimtrappedandcantgetout", &F);
    CallInst::Create(Intrinsic::getDeclaration(F.getParent(), Intrinsic::trap),
                     {}, "", trapBlock);

    if (F.getReturnType()->getTypeID() == Type::VoidTyID)
      ReturnInst::Create(F.getContext(), nullptr, trapBlock);
    else
      ReturnInst::Create(F.getContext(),
                         Constant::getNullValue(F.getReturnType()), trapBlock);
    instructionsAdded += 2;

    // Step 4: bounds check on writes
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *SI = dyn_cast<StoreInst>(&I)) {
          if (ourStores.contains(SI))
            continue;
          auto *ptr = SI->getPointerOperand();
          if (!bounds.contains(ptr))
            continue;
          if (PRINTDEBUG)
            dbgs() << "Found Store " << *SI << "\n";
          if (PRINTDEBUG)
            dbgs() << "ptr = " << *ptr << "\n";
          auto [lower, size] = bounds.lookup(ptr);
          if (PRINTDEBUG)
            dbgs() << "bounds = " << *lower << " " << *size << "\n";

          // if ptr < lower, trap
          auto *cmp = new ICmpInst(SI, ICmpInst::ICMP_ULT, ptr, lower);
          auto *new_origBB = BB.splitBasicBlockBefore(SI, "");
          auto *new_orig_target = new_origBB->getSingleSuccessor();
          auto *br = BranchInst::Create(trapBlock, new_orig_target, cmp);
          ReplaceInstWithInst(new_origBB->getTerminator(), br);

          // if ptr - lower >= size, trap
          auto *ptrInt =
              new PtrToIntInst(ptr, Type::getInt64Ty(F.getContext()), "", SI);
          auto *lowerInt =
              new PtrToIntInst(lower, Type::getInt64Ty(F.getContext()), "", SI);
          auto *diff = BinaryOperator::Create(Instruction::Sub, ptrInt,
                                              lowerInt, "", SI);
          auto *cmp2 = new ICmpInst(SI, ICmpInst::ICMP_UGE, diff, size);
          // split BB
          auto *new_origBB2 = BB.splitBasicBlockBefore(SI, "");
          auto *new_orig_target2 = new_origBB2->getSingleSuccessor();
          auto *br2 = BranchInst::Create(trapBlock, new_orig_target2, cmp2);
          ReplaceInstWithInst(new_origBB2->getTerminator(), br2);

          instructionsAdded += 8;

          // // if ptr - lower >= size, trap
          // auto *ptrInt =
          //     new PtrToIntInst(ptr, Type::getInt64Ty(F.getContext()), "",
          //     SI);
          // auto *lowerInt =
          //     new PtrToIntInst(lower, Type::getInt64Ty(F.getContext()), "",
          //     SI);
          // auto *diff = BinaryOperator::Create(Instruction::Sub, ptrInt,
          //                                     lowerInt, "", SI);
          // auto *cmp = new ICmpInst(SI, ICmpInst::ICMP_UGE, diff, size);
          // // split BB
          // auto *new_origBB = BB.splitBasicBlockBefore(SI, "");
          // if (PRINTDEBUG)
          //   dbgs() << "AFTER SPLITTING"
          //          << "\n";
          // if (PRINTDEBUG)
          //   dbgs() << F << "\n";
          // auto *new_orig_target = new_origBB->getSingleSuccessor();
          // auto *br = BranchInst::Create(trapBlock, new_orig_target, cmp);
          // ReplaceInstWithInst(new_origBB->getTerminator(), br);
          // instructionsAdded += 5;
        }
      }
    }

    dbgs() << instructionsAdded << " instructions added\n";

    return PreservedAnalyses::all();
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