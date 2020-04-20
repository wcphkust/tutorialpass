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

/*----------------------------------------------------------
*  Implementation of SingleBasicBlockLivenessInfo
*  Description: Record liveness of variables in each basic block
* ----------------------------------------------------------
*/


/*----------------------------------------------------------
* Implementation of BasicBlockWorkList
* Description: Record the calculation status of basic block
*----------------------------------------------------------
*/


/*
 * BasicBlockWorkList constructor
 * Function: create the worklist of basic blocks, initialize by adding the exit basic block
 */
BasicBlockWorkList::BasicBlockWorkList(Function &F) {
    auto it = F.getBasicBlockList().rbegin();
    BasicBlock* exitBB = &(*it);
    BasicBlockList.push_back(exitBB);
    /*
     * for (BasicBlock* bb = F.getBasicBlockList().rbegin(); bb != F.getBasicBlockList().rend(); bb++) {
        BasicBlockList.push(bb);
        state[bb] = 0;
    }
     */
}

/*
 * Get Basic Block Status
 * Return:
 *   0 for non-calculated, 1 for calculated, 2 for to be recalculated
 */
int BasicBlockWorkList::getBasicBlockStatus(BasicBlock* bb) {
    return state[bb];
}

/*
 * Add basic block to worklist
 */
void BasicBlockWorkList::pushBasicBlockToWorkList(BasicBlock *bb) {
    bool isInWorkList = false;
    for (auto it = BasicBlockList.begin(); it != BasicBlockList.end(); it++) {
        if (bb == *it) {
            isInWorkList = true;
        }
    }

    if (not isInWorkList) {
        BasicBlockList.push_back(bb);
    }
}

/*
 * Remove basic block from worklist
 */
void BasicBlockWorkList::popBasicBlockToWorkList() {
    BasicBlockList.pop_front();
}

//Merge two bitvectors
BitVectorMap BasicBlockWorkList::mergeBitVector(BitVectorMap bv1, BitVectorMap bv2, bool approx_para) {
    BitVectorMap bv;
    for (auto it = bv1.begin(); it != bv1.end(); it++) {
        if (approx_para) {
            bv[it->first] = (bv1[it->first] or bv2[it->first]);
        } else {
            bv[it->first] = (bv1[it->first] and bv2[it->first]);
        }
    }
    return bv;
}

/*
 * Merge the bitvectors at the entry of the basic block, propagated from the successors backward
 * Parameter
 *   bb: Basic Block to be calculated
 *   bv: bit vector propagated backward from a successor
 *   approx_para: true for may analysis, false for must analysis, default to true
 */
void BasicBlockWorkList::mergeTailBVMap(BasicBlock* bb, BitVectorMap bv, bool approx_para) {
    auto it = tailBVMap.find(bb);
    if (it == tailBVMap.end()) {
        tailBVMap[bb] = bv;
    } else {
        tailBVMap[bb] = mergeBitVector(tailBVMap[bb], bv, approx_para);
    }
}

/*
 * Judge whether the worklist is empty or not
 * Return: 1 for empty, 0 for non-empty
 */
bool BasicBlockWorkList::BasicBlockWorkList::isEmpty() {
    return BasicBlockList.empty();
}

//----------------------------------------------------------
// Implementation of LiveVariableInBranch
//----------------------------------------------------------

/*
 * Main function of Live Variable Analysis in Branch, and can be generalized to loop
 */
bool LiveVariableInBranch::runOnFunction(llvm::Function &F) {

    // Collect the basic blocks in the function


    // Step 0: Initialize the tail statement of the last basic block


    // Fixed point algorithm
    /*
     * Step 1: Update one basic block at the head of the queue
     * Step 2: Propagate to the previous one, initialize the bitvector of tail statement
     */

    // Output the analysis result

    return false;
}


/*
 * Judege the equal relation of two bit vector
 * Parameter: BitVectorMap bv1, bv2
 * Return: true if bv1=bv2, otherwise false
*/
 bool LiveVariableInBranch::isEqualBitVector(BitVectorMap bv1, BitVectorMap bv2) {

 }


/*
 * Judge whether the fixed point has been reached
 * Parameter:
 *   bb: the basic block to be checked
 *   connectBV: the incoming bit vector
 * Return:
 *   true: FP has reached; false: otherwise
 */
bool LiveVariableInBranch::hasReachedFixedPoint(BasicBlock* bb, BitVectorMap connectBV) {

}

/*
 * Get the liveness info at each location in a single basic block and store the liveness info in the map
 * Parameter:
 *  bb: the pointer of basic block
 *  bv: the initial liveness state, i.e. the liveness info of the last statement
 * Return: the liveness info the first statement
 */
BitVectorMap LiveVariableInBranch::getLivenessInSingleBB(BasicBlock *bb, BitVectorMap bv) {
    BitVectorBase KillBase, GenBase;
    BitVectorMap BlockRes;


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
                                        PM.add(new VariableInBB());
                                        PM.add(new LiveVariableInBranch());
                                    });
