//
// Created by Sunshine on 16/3/2020.
// Reference: https://stackoverflow.com/questions/47978363/get-variable-name-in-llvm-pass
//

#include <iostream>
#include <fstream>
#include <set>
#include <stack>
#include <map>
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "LiveVariableInBranch.h"
#include "VariableInBB.h"


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
    BBLiveVec.push(bv);
}

/*
 * Add the liveness info of current statement to BBLiveVec
 */
void SingleBasicBlockLivenessInfo::pushBitVector(BitVectorMap bv) {
    BBLiveVec.push(bv);
}

/*
 * Get the liveness of first statement
 */
BitVectorMap SingleBasicBlockLivenessInfo::getBasicHeadLiveInfo() {
    BitVectorMap headBV = BBLiveVec.back();
    return headBV;
}

/*
 * Get the liveness of last statement
 */
BitVectorMap SingleBasicBlockLivenessInfo::getBasicTailLiveInfo() {
    BitVectorMap tailBV = BBLiveVec.front();
    return tailBV;
}

//----------------------------------------------------------
// Implementation of BasicBlockWorkList
// Description: Record the calculation status of basic block
//----------------------------------------------------------

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
BitVectorMap BasicBlockWorkList::mergeBitVector(BitVectorMap &bv1, BitVectorMap &bv2, bool approx_para) {
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
void BasicBlockWorkList::mergeTailBVMap(BasicBlock* bb, BitVectorMap &bv, bool approx_para) {
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
    set<string> varSet = getAnalysis<VariableInBB>().getVarSet();

    //construct the BitVectorBase
    for (set<string>::iterator it = varSet.begin(); it != varSet.end(); ++it) {
        vectorBase.push_back(*it);
    }

    /*Step 0: initialize the tail statement of the last basic block*/
    BitVectorMap bv = getLivenessInLastBB(F);

    //collect the basic block and initialize BasicBlockFlag
    BasicBlockWorkList BBWorkList = BasicBlockWorkList(F);

    BasicBlock* exitBB = &(*F.getBasicBlockList().rbegin()); //error prone
    BBWorkList.tailBVMap[exitBB] = generateEmptyBitVector();
    BBWorkList.state[exitBB] = 1;

    //Worklist algorithm
    /*
     * Step 1: Update one basic block at the head of the queue
     * Step 2: Propagate to the previous one, initialize the bitvector of tail statement
     */
    int iterNum = 0;
    while (not BBWorkList.isEmpty()) {
        iterNum++;
        errs() << "Basic Block Num: " << iterNum << "\n";
        BasicBlock* bb = BBWorkList.BasicBlockList.front();
        errs() << "Basic Block Name: " << bb->getName() << "\n";
        BitVectorMap connectBV = getLivenessInSingleBB(bb, BBWorkList.tailBVMap[bb]);

        for (auto it = pred_begin(bb), et = pred_end(bb); it != et; ++it) {
            BasicBlock* predBB = *it;
            if (not hasReachedFixedPoint(predBB, connectBV)) {
                BBWorkList.pushBasicBlockToWorkList(predBB);
                BBWorkList.mergeTailBVMap(predBB, connectBV);
            }
        }
        BBWorkList.popBasicBlockToWorkList();
        BBWorkList.state[bb] = 0;
    }

    errs() << "---------------------------------" << "\n";
    errs() << "The iteration number of worklist is " << iterNum << "\n";
    errs() << "---------------------------------" << "\n";
    printLiveVariableInBranchResult(F.getName());

    return false;
}


/*
 * Judege the equal relation of two bit vector
 * Parameter: BitVectorMap bv1, bv2
 * Return: true if bv1=bv2, otherwise false
*/
 bool LiveVariableInBranch::isEqualBitVector(BitVectorMap &bv1, BitVectorMap &bv2) {
     for (auto it = bv1.begin(); it != bv1.end(); it++) {
         if (bv1[it->first] != bv2[it->first]) {
             return false;
         }
     }
     return true;
 }


/*
 * Judge whether the fixed point has been reached
 * Parameter:
 *   bb: the basic block to be checked
 *   connectBV: the incoming bit vector
 * Return:
 *   true: FP has reached; false: otherwise
 */
bool LiveVariableInBranch::hasReachedFixedPoint(BasicBlock* bb, BitVectorMap &connectBV) {
    if (BasicBlockLivenessInfo.find(bb) == BasicBlockLivenessInfo.end()) {
        return false;
    }
    SingleBasicBlockLivenessInfo info = BasicBlockLivenessInfo[bb];
    BitVectorMap originConnectBV = info.getBasicTailLiveInfo();
    if (isEqualBitVector(originConnectBV, connectBV)) {
        return true;
    }

    return false;
}

/*
 * Get the liveness info at each location in a single basic block and store the liveness info in the map
 * Parameter:
 *  bb: the pointer of basic block
 *  bv: the initial liveness state, i.e. the liveness info of the last statement
 * Return: the liveness info the first statement
 */
BitVectorMap LiveVariableInBranch::getLivenessInSingleBB(BasicBlock *bb, BitVectorMap &bv) {
    SingleBasicBlockLivenessInfo info = SingleBasicBlockLivenessInfo(bv);
    BitVectorBase KillBase, GenBase;

    for (auto inst = bb->getInstList().rbegin(); inst != bb->getInstList().rend(); inst++) {
        //skip AllocaInst
        if (llvm::isa<llvm::AllocaInst>(*inst)) {
            continue;
        }

        if (llvm::isa<llvm::StoreInst>(*inst)) {
            //Store
            errs() << "-----------------------------" << "\n";
            errs() << "Store instruction" << "\n";
            errs() << (*inst) << "\n";
            auto op = inst->op_begin();
            errs() << "Gen:" << op->get()->getName() << "\n";
            GenBase.push_back(op->get()->getName());
            op++;
            errs() << "Kill:" << op->get()->getName() << "\n";
            KillBase.push_back(op->get()->getName());
            errs() << "-----------------------------" << "\n";
        } else if (llvm::isa<llvm::LoadInst>(*inst)) {
            //Load
            errs() << "-----------------------------" << "\n";
            errs() << "Load instruction" << "\n";
            errs() << (*inst) << "\n";
            auto op = inst->op_begin();
            errs() << "Kill:" << inst->getName() << "\n";
            KillBase.push_back(inst->getName());
            errs() << "Gen:" << op->get()->getName() << "\n";
            GenBase.push_back(op->get()->getName());
            errs() << "-----------------------------" << "\n";
        } else continue;

        BitVectorMap headBv = info.getBasicHeadLiveInfo();
        BitVectorMap newBv = transferFunction(headBv, KillBase, GenBase);
        KillBase.clear();
        GenBase.clear();

        info.pushBitVector(newBv);
        errs() << "line number: " << inst->getDebugLoc().getLine();
        lineInfo[inst->getDebugLoc().getLine()] = newBv;
    }

    //store info in the BasicBlockLivenessInfo
    BasicBlockLivenessInfo[bb] = info;

    return info.BBLiveVec.back();
}

/*
 * Get the liveness info at each location in the exit block and store the liveness info in the map
 * Parameter:
 *  bb: the pointer of exit block
 * Return: the liveness info the first statement
 */
BitVectorMap LiveVariableInBranch::getLivenessInLastBB(Function &F) {
    BitVectorMap bv = generateEmptyBitVector();
    BasicBlock* bb = &(*F.getBasicBlockList().rbegin());
    BitVectorMap newBv = getLivenessInSingleBB(bb, bv);
    return newBv;
}

/*
 * Get empty bit vector storing zero vector
 * Return: the empty bit vector
 */
BitVectorMap LiveVariableInBranch::generateEmptyBitVector() {
    BitVectorMap bv;
    for (int i = 0; i < vectorBase.size(); i++) {
        bv[vectorBase[i]] = 0;
    }
    return bv;
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
BitVectorMap LiveVariableInBranch::transferFunction(BitVectorMap &bv, BitVectorBase &KillBase, BitVectorBase &GenBase) {
    for (int i = 0; i < KillBase.size(); i++) {
        if (find(vectorBase.begin(), vectorBase.end(), KillBase[i]) != vectorBase.end()) {
            bv[KillBase[i]] = 0;
        }
    }
    for (int i = 0; i < GenBase.size(); i++) {
        if (find(vectorBase.begin(), vectorBase.end(), GenBase[i]) != vectorBase.end()) {
            bv[GenBase[i]] = 1;
        }
    }

    errs() << "--------------------------------" << "\n";
    for (map<string, int>::iterator it = bv.begin(); it != bv.end(); it++) {
        errs() << it->first << ":" << it->second << "\n";
    }
    errs() << "--------------------------------" << "\n";

    return bv;
}

/*
 * Print the result
 */
void LiveVariableInBranch::printLiveVariableInBranchResult(StringRef FuncName) {
    errs() << "================================================="
           << "\n";
    errs() << "LLVM-TUTOR: Live Variable results for `" << FuncName
           << "`\n";
    errs() << "=================================================\n";

    auto line_it = lineInfo.begin();
    line_it++;
    ofstream fout("testoutput.txt");
    for ( ; line_it != lineInfo.end(); line_it++) {
        errs() << "line: " << line_it->first << " {";
        fout << "line:" << line_it->first << " {";
        for (auto bit_it = line_it->second.begin(); bit_it != line_it->second.end(); bit_it++) {
            if (bit_it->second == 1) {
                errs() << bit_it->first << " ";
                fout << bit_it->first << " ";
            }
        }
        errs() << "}" << "\n";
        fout << "}" << "\n";
    }

    errs() << "------------------------------" << "\n";
    errs() << "-------------------------------------------------" << "\n\n";
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