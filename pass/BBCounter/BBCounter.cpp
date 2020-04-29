//
// Created by Sunshine on 13/3/2020.
//

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "BBCounter.h"

using namespace llvm;

char LegacyBBCounter::ID = 0;

//----------------------------------------------------------
// BBCounter implementation
//----------------------------------------------------------

/*
 * Count the basic blocks in all the functions
 */
BBCounter::Result BBCounter::generateBBCount(Function &Func) {
    BBCounter::Result BBCount = 0;
    for (auto &BB: Func) {
        BBCount++;
    }

    return BBCount;
}

/*
 * Count the basic blocks in a function
 */
PreservedAnalyses BBCounter::run(llvm::Function &Func, llvm::FunctionAnalysisManager &) {
    BBCounter::Result BBCount = generateBBCount(Func);
    printBBCounterResult(BBCount, Func.getName());

    return llvm::PreservedAnalyses::all();
}

/*
 * Count the basic blocks in all the functions
 */
bool LegacyBBCounter::runOnFunction(llvm::Function &Func) {
    BBCounter::Result countResult = Impl.generateBBCount(Func);
    printBBCounterResult(countResult, Func.getName());

    return false;
}

/*
 * Print the counting result of basic blocks
 */
void printBBCounterResult(const ResultBBCounter &BBCount,
                              StringRef FuncName) {
    llvm::errs() << "================================================="
                 << "\n";
    llvm::errs() << "LLVM-TUTOR: BBCounter results for `" << FuncName
                 << "`\n";
    llvm::errs() << "=================================================\n";
    const char *str1 = "Basic Block";
    const char *str2 = "#N TIMES USED";
    llvm::errs() << format("%-20s %-10s\n", str1, str2);
    llvm::errs() << "-------------------------------------------------"
                 << "\n";
    llvm::errs() << BBCount
                 << "\n";
    llvm::errs() << "-------------------------------------------------"
                 << "\n\n";
}

/*
 * New PM Registration
 */
llvm::PassPluginLibraryInfo getBasicBlockCounterPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "BasicBlockCounter", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, FunctionPassManager &FPM,
                           ArrayRef<PassBuilder::PipelineElement>) {
                            if (Name == "basic-block-counter") {
                                FPM.addPass(BBCounter());
                                return true;
                            }
                            return false;
                        });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getBasicBlockCounterPluginInfo();
}

// Register the pass - with this 'opt' will be able to recognize
// BasicBlock when added to the pass pipeline on the command line, i.e.
// via '--basic-block-count'
static RegisterPass<LegacyBBCounter>
        X("legacy-bb-counter", "BBCounter Pass",
          true, // This pass doesn't modify the CFG => true
          false // This pass is not a pure analysis pass => false
);

#ifdef LT_OPT_PIPELINE_REG
// Register BBCounter as a step of an existing pipeline. The insertion
// point is set to 'EP_EarlyAsPossible', which means that BBCounter
// will be run automatically at '-O{0|1|2|3}'.
//
// NOTE: this trips 'opt' installed via HomeBrew (Mac OS) and apt-get (Ubuntu).
// It's a known issues:
//    https://github.com/sampsyo/llvm-pass-skeleton/issues/7
//    https://bugs.llvm.org/show_bug.cgi?id=39321
static llvm::RegisterStandardPasses
    RegisterBBCounter(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                          [](const llvm::PassManagerBuilder &Builder,
                             llvm::legacy::PassManagerBase &PM) {
                            PM.add(new LegacyBBCounter());
                          });
#endif