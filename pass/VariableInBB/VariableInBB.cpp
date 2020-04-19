//
// Created by Sunshine on 16/3/2020.
//

#include "../../include/VariableInBB.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

char VariableInBB::ID = 0;

//----------------------------------------------------------
// VariableinBB implementation
//----------------------------------------------------------

//collect the defined variable which can be accessed in each basic block
bool VariableInBB::runOnFunction(Function &F) {
    for (const BasicBlock &BB : F){
        for (const Instruction &AI : BB) {
            if (llvm::isa<llvm::AllocaInst>(AI)) {
//                errs() << AI << "\n";
//                errs() << "string: " << AI.getName() << "\n";
                varSet.insert(AI.getName());
            }
        }
    }
    printVariableInBBResult(varSet, F.getName());
    return false;
}

//return the set of variables in the basic block
set<string> VariableInBB::getVarSet() {
    return varSet;
}

/*
 * Print the counting result of basic blocks
 */
void printVariableInBBResult(const set<string> varSet, StringRef FuncName) {
    errs() << "================================================="
                 << "\n";
    errs() << "LLVM-TUTOR: Variables In Basic Block results for `" << FuncName
                 << "`\n";
    errs() << "=================================================\n";
    for (set<string>::iterator it = varSet.begin(); it != varSet.end(); it++) {
        errs() << *it << "\n";
    }
    errs() << "-------------------------------------------------" << "\n\n";
}

static RegisterPass<VariableInBB> X("var-in-bb", "VariableInBB Pass",
        true, // This pass doesn't modify the CFG => true
        false // This pass is not a pure analysis pass => false
);

static RegisterStandardPasses
        registerBBinLoopCounterPass(PassManagerBuilder::EP_EarlyAsPossible,
                        [](const PassManagerBuilder &Builder,
                           legacy::PassManagerBase &PM) {
                            PM.add(new VariableInBB());
                        });