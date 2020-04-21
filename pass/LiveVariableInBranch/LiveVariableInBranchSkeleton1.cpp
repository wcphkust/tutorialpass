//
// Created by Sunshine on 16/3/2020.
// Reference: https://stackoverflow.com/questions/47978363/get-variable-name-in-llvm-pass
//

#include <iostream>
#include <set>
#include <stack>
#include <vector>
#include <map>
#include <queue>
#include <utility>
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "../../include/LiveVariableInBranch.h"
#include "../../include/VariableInBB.h"

using namespace std;
using namespace llvm;

char LiveVariableInBranch::ID = 0;

//----------------------------------------------------------
// Implementation of SingleBasicBlockLivenessInfo
// Description: Record liveness of variables in each basic block
//----------------------------------------------------------

/*
 * SingleBasicBlockLivenessInfo constructor
 * Ininitialize by inserting the bit vector of the last statement
 */
SingleBasicBlockLivenessInfo::SingleBasicBlockLivenessInfo(BitVectorMap bv) {
    /* TODO */
}

/*
 * Add the liveness info of current statement to BBLiveVec
 */
void SingleBasicBlockLivenessInfo::pushBitVector(BitVectorMap bv) {
    /* TODO */
}

/*
 * Get the liveness of first statement
 */
BitVectorMap SingleBasicBlockLivenessInfo::getBasicHeadLiveInfo() {
    /* TODO */
}

/*
 * Get the liveness of last statement
 */
BitVectorMap SingleBasicBlockLivenessInfo::getBasicTailLiveInfo() {
    /* TODO */
}


/*----------------------------------------------------------
* Implementation of LiveVariableInBranch
*----------------------------------------------------------
*/


// Main function of Live Variable Analysis in Branch
bool LiveVariableInBranch::runOnFunction(llvm::Function &F) {
    /* TODO */

    printLiveVariableInBranchResult(F.getName());
    return false;
}

/*
 * Get the liveness info at each location in a single basic block and store the liveness info in the map
 * Parameter:
 *  bb: the pointer of basic block
 *  bv: the initial liveness state, i.e. the liveness info of the last statement
 * Return: the liveness info the first statement in the basic block *bb
 */
BitVectorMap LiveVariableInBranch::getLivenessInSingleBB(BasicBlock *bb, BitVectorMap bv) {
    BitVectorBase KillBase, GenBase;
    BitVectorMap BlockRes;

    /* TODO */

    return BlockRes;
}


/*
 * Update the liveness by Kill and Gen set
 * Formula: f(x) = (x - Kill) \cup Gen
 * Parameter:
 *   bv: the liveness info at the entry of the statement
 *   KillBase, GenBase: Kill set and Gen set
 * Return:
 *   The updated liveness info after kill and gen opertion
 */
void LiveVariableInBranch::transferFunction(BitVectorMap& bv, BitVectorBase& KillBase, BitVectorBase& GenBase) {
    /* TODO */
}

/*
 * Print the result of the given function
 * Parameters:
 *    @FuncName: name of the function wanted
 */
void LiveVariableInBranch::printLiveVariableInBranchResult(StringRef FuncName) {
    errs() << "================================================="
           << "\n"
           << "LLVM-TUTOR: Live Variable results for `" << FuncName
           << "\n"
           << "=================================================\n";
    /* TODO */

}

/*
 * This method tells LLVM which other passes we need to execute properly
 */
void LiveVariableInBranch::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<VariableInBB>();
    AU.setPreservesAll();
}

static RegisterPass<LiveVariableInBranch> X("live-var-in-branch", "LiveVariableInBranch Pass",
                                        true, // This pass doesn't modify the CFG => true
                                        false // This pass is not a pure analysis pass => false
);

static llvm::RegisterStandardPasses
        registerBBinLoopCounterPass(PassManagerBuilder::EP_EarlyAsPossible,
                                    [](const PassManagerBuilder &Builder,
                                       legacy::PassManagerBase &PM) {
                                        PM.add(new LiveVariableInBranch());
                                    });
