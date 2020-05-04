//
// Created by Sunshine on 14/3/2020.
//

//
// Created by Sunshine on 13/3/2020.
//

#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "pass/BBinLoopCounter.h"

using namespace llvm;

char BBinLoopCounter::ID = 0;

//----------------------------------------------------------
// BBinLoopCounter implementation
//----------------------------------------------------------

//This method tells LLVM which other passes we need to execute properly
void BBinLoopCounter::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.setPreservesAll();
}

/*
 * Count the basic blocks in the loops
 */
bool BBinLoopCounter::runOnFunction(llvm::Function &F) {
    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    errs() << "Get Loop Info Success" << "\n";
    int loopCounter = 0;
    errs() << F.getName() + "\n";

    for (LoopInfo::iterator i = LI.begin(), e = LI.end(); i != e; ++i) {
        errs() << F.getName() + "\n";
        Loop *L = *i;
        int bbCounter = 0;
        loopCounter++;
        for (Loop::block_iterator bb = L->block_begin(); bb != L->block_end(); ++bb) {
            bbCounter += 1;
        }
        errs() << "Loop";
        errs() << loopCounter;
        errs() << ":#BBs =";
        errs() << bbCounter;
        errs() << "\n";
    }
    return false;
}

static RegisterPass<BBinLoopCounter> X("bb-in-loop-counter", "BBinLoopCounter Pass",
          true, // This pass doesn't modify the CFG => true
          false // This pass is not a pure analysis pass => false
);

static llvm::RegisterStandardPasses
        registerBBinLoopCounterPass(PassManagerBuilder::EP_EarlyAsPossible,
                          [](const PassManagerBuilder &Builder,
                             legacy::PassManagerBase &PM) {
                              PM.add(new LoopInfoWrapperPass());
                              PM.add(new BBinLoopCounter());
                          });