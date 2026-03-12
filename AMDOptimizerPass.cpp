#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/ConstantFolding.h" 
#include "llvm/IR/DataLayout.h"

using namespace llvm;

namespace {
struct AMDOptimizerPass : public PassInfoMixin<AMDOptimizerPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        bool changed = false;
        const DataLayout &DL = F.getParent()->getDataLayout();

        for (auto &BB : F) {
            for (auto Inst = BB.begin(); Inst != BB.end(); ) {
                Instruction &I = *Inst++;

                // This LLVM helper function handles the "looking back" through registers 
                // to see if the operation can be simplified to a constant.
                if (Constant *C = ConstantFoldInstruction(&I, DL)) {
                    errs() << "AMD Optimizer: Folded instruction: " << I << "\n";
                    I.replaceAllUsesWith(C);
                    I.eraseFromParent();
                    changed = true;
                } 
                // Dead Code Elimination: If it's useless, trash it.
                else if (I.use_empty() && !I.isTerminator() && !I.mayHaveSideEffects()) {
                    errs() << "AMD Optimizer: Deleted dead code: " << I << "\n";
                    I.eraseFromParent();
                    changed = true;
                }
            }
        }
        return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "AMDOptimizer", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "amd-opt") {
                            FPM.addPass(AMDOptimizerPass());
                            return true;
                        }
                        return false;
                    });
            }};
}