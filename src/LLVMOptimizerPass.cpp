#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

using namespace llvm;

namespace {

// Optimise a single function: mem2reg then constant-fold + DCE.
static bool optimizeFunction(Function &F, FunctionAnalysisManager &FAM) {
    bool changed = false;

    // Promote alloca/store/load patterns to SSA so ConstantFoldInstruction
    // can see constant values that were written through stack slots.
    SmallVector<AllocaInst *, 8> Allocas;
    for (Instruction &I : F.getEntryBlock())
        if (auto *AI = dyn_cast<AllocaInst>(&I))
            if (isAllocaPromotable(AI))
                Allocas.push_back(AI);

    if (!Allocas.empty()) {
        auto &DT = FAM.getResult<DominatorTreeAnalysis>(F);
        auto &AC = FAM.getResult<AssumptionAnalysis>(F);
        PromoteMemToReg(Allocas, DT, &AC);
        changed = true;
    }

    const DataLayout &DL = F.getParent()->getDataLayout();

    for (auto &BB : F) {
        for (auto Inst = BB.begin(); Inst != BB.end(); ) {
            Instruction &I = *Inst++;

            // Fold any instruction whose operands are all constants.
            if (Constant *C = ConstantFoldInstruction(&I, DL)) {
                errs() << "LLVM Optimizer: Folded instruction: " << I << "\n";
                I.replaceAllUsesWith(C);
                I.eraseFromParent();
                changed = true;
            }
            // Dead Code Elimination: remove side-effect-free unused instructions.
            else if (I.use_empty() && !I.isTerminator() && !I.mayHaveSideEffects()) {
                errs() << "LLVM Optimizer: Deleted dead code: " << I << "\n";
                I.eraseFromParent();
                changed = true;
            }
        }
    }
    return changed;
}

// Module pass so we can strip optnone from every function before the pass
// manager would otherwise skip them. Clang -O0 marks all functions optnone,
// which prevents any transformation pass from running.
struct LLVMOptimizerPass : public PassInfoMixin<LLVMOptimizerPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
        bool changed = false;
        auto &FAMProxy = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M);
        auto &FAM = FAMProxy.getManager();

        for (Function &F : M) {
            if (F.isDeclaration())
                continue;

            // Strip optnone so our passes can actually run on -O0 compiled IR.
            if (F.hasFnAttribute(Attribute::OptimizeNone)) {
                F.removeFnAttr(Attribute::OptimizeNone);
                changed = true;
            }

            changed |= optimizeFunction(F, FAM);
        }

        return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "LLVMOptimizer", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, ModulePassManager &MPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "llvm-opt") {
                            MPM.addPass(LLVMOptimizerPass());
                            return true;
                        }
                        return false;
                    });
            }};
}