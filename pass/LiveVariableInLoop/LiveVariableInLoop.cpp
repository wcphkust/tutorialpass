//
// Created by chasen on 29/4/20.
//

#include <iostream>
#include <fstream>
#include <set>
#include <stack>
#include <map>
#include "llvm/IR/CFG.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "LiveVariableInLoop.h"
#include "VariableInBB.h"

using namespace std;
using namespace llvm;

char LiveVariableInLoop::ID = 0;

//----------------------------------------------------------
// Implementation of InstWorkList
// Description: Record the calculation status of instruction
//----------------------------------------------------------

/*
 * InstWorkList constructor
 * Function &F: the function from which worklist of instruction is created
 * BitVectorBase varIndex: the mapping from variables to index
 * isForward: is forward-analysis
 *   true: initialize by adding the first instruction of entry
 *   false: initialize by adding the first instruction of entry
 */
InstWorkList::InstWorkList(Function &F, BitVectorBase pVarIndex, bool isForward, bool isMay) {
    direction = (isForward ? FORWARD : BACKWORD);
    approxMode = (isMay ? MAY : MUST);

    if (isForward) {
        auto it = F.getBasicBlockList().begin();
        BasicBlock* entryBB = &(*it);
        Instruction* firstInst = &(entryBB->front());
        instPreFactMap[firstInst] = BitVector(pVarIndex.size(), false);
        workList.push_back(firstInst);
    } else {
        auto it = F.getBasicBlockList().rbegin();
        BasicBlock* exitBB = &(*it);
        Instruction* lastInst = &(exitBB->back());
        instPostFactMap[lastInst] = BitVector(pVarIndex.size(), false);
        workList.push_back(lastInst);
    }
}

/*
 * Kill-Gen transfer function
 * Kill, Gen should be calculated before the invocation in the caller
 */
BitVector InstWorkList::transferFunction(BitVector &bv, BitVector &Kill, BitVector &Gen) {
    BitVector resBv(bv);
    resBv &=  Kill.flip();
    resBv |= Gen;
    return resBv;
}

/*
 * Kill-Gen transfer function
 * KillBase, GenBase should be passed before the invocation in the caller
 */
BitVector LiveVariableInLoop::constructBitVector(BitVectorBase &base) {
    BitVector bv(varIndex.size(), false);

    for (auto it = base.begin(); it != base.end(); it++) {
        if (varIndex.find(it->first) != varIndex.end()) {
            bv.set(varIndex[it->first]);
        }
    }
    return bv;
}

/*
 * Join operator
 */
BitVector InstWorkList::join(BitVector &bv1, BitVector &bv2) {
    BitVector bv = bv1;
    if (approxMode == MAY) {
        bv |= bv2;
    } else {
        //approxMode == MUST
        bv &= bv2;
    }
    return bv;
}


/*
 * Fixed point checker
 * bv: the state of the instruction after transfermation
 * inst: instruction pointer
 */
bool InstWorkList::isFixedPoint(BitVector bv, Instruction* inst) {
    map<Instruction*, BitVector>* factMap = (direction == BACKWORD ? &instPreFactMap : &instPostFactMap);
    if (factMap->find(inst) == factMap->end()) {
        return false;
    }
    // Judge whether the original state is covered by the current one(bv)
    BitVector oldBv = (*factMap)[inst];
    BitVector joinBv = join(oldBv, bv);
    return (joinBv == oldBv);
}

/*
 * Add instruction to worklist
 * Return false if inst has existed, false otherwise
 */
bool InstWorkList::pushInstToWorkList(Instruction *inst) {
    for (auto & it : workList) {
        if (inst == it) {
            return false;
        }
    }
    workList.push_back(inst);
    return true;
}

/*
 * Add the instructions which depend on inst
 * Update the pre/post-state based on the post/pre-state of inst
 */
void InstWorkList::pushDepsInstToWorkList(Instruction *inst, InstDepsFunc deps) {
    deque<Instruction*> depInsts = deps(inst);
    for (auto & depInst : depInsts) {
        pushInstToWorkList(depInst);
        if (direction == BACKWORD) {
            BitVector postBv = instPreFactMap[inst];
            insertPostBitVector(depInst, postBv);
        } else {
            //FORWARD
            BitVector preBv = instPostFactMap[inst];
            insertPreBitVector(depInst, preBv);
        }
    }
}

/*
 * Remove Instruction from worklist
 */
void InstWorkList::popInstToWorkList() {
    return workList.pop_front();
}

/*
 * Get the head of worklist
 */
Instruction* InstWorkList::getWorkListHead() {
    return workList.front();
}

/*
 * Insert bit vector into PreFactMap
 */
void InstWorkList::insertPreBitVector(Instruction* inst, BitVector bv) {
    if (instPreFactMap.count(inst) == 0) {
        instPreFactMap[inst] = bv;
    } else {
        instPreFactMap[inst] = join(instPreFactMap[inst], bv);
    }
}

/*
 * Insert bit vector into PostFactMap
 */
void InstWorkList::insertPostBitVector(Instruction* inst, BitVector bv) {
    if (instPostFactMap.count(inst) == 0) {
        instPostFactMap[inst] = bv;
    } else {
        instPostFactMap[inst] = join(instPostFactMap[inst], bv);
    }
}

/*
 * Judge whether the worklist is empty or not
 * Return: 1 for empty, 0 for non-empty
 */
bool InstWorkList::isEmpty() {
    return workList.empty();
}

//----------------------------------------------------------
// Implementation of LiveVariableInLoop
//---------------------------------------------------------—

/*
 * Main function of Live Variable Analysis in Branch, and can be generalized to loop
 */
bool LiveVariableInLoop::runOnFunction(llvm::Function &F) {
    set<string> varSet = getAnalysis<VariableInBB>().getVarSet();

    //construct the BitVectorBase
    int index = 0;
    for (auto it = varSet.begin(); it != varSet.end(); it++) {
        varIndex[*it] = index;
        index++;
    }

    //collect the instruction at the exit node and initialize the worklist
    LVAWorkList = InstWorkList(F, varIndex,  false, true);

    //Worklist algorithm
    int iterNum = 0;
    while (not LVAWorkList.isEmpty()) {
        iterNum++;
        Instruction* inst = LVAWorkList.getWorkListHead();

        BitVector postBv = LVAWorkList.instPostFactMap[inst];
        BitVector preBv;

        if (llvm::isa<llvm::StoreInst>(*inst) or llvm::isa<llvm::LoadInst>(*inst)) {
            preBv = transferFunction(inst, postBv);
        } else {
            preBv = postBv;
        }
        
        bool isFixedPointReached = LVAWorkList.isFixedPoint(preBv, inst);

        if (not isFixedPointReached) {
            LVAWorkList.insertPreBitVector(inst, preBv);
            LVAWorkList.pushDepsInstToWorkList(inst, predDepsFunc);
        }
        LVAWorkList.popInstToWorkList();
    }

    errs() << "---------------------------------" << "\n";
    errs() << "The iteration number of worklist is " << iterNum << "\n";
    errs() << "---------------------------------" << "\n";

    getLineLivenessInfo();
    printLiveVariableInLoopResult(F.getName());

    return false;
}

/*
 * Deps function: get the predecessor of the instruction
 * Parameter:
 *   inst： the current instruction
 * Return：the list of instructions
 */
deque<Instruction*> LiveVariableInLoop::predDepsFunc(Instruction* inst) {
    deque<Instruction*> predecessor;
    BasicBlock* bb = inst->getParent();

    if (inst == &(bb->front())) {
        for (auto it = pred_begin(bb); it != pred_end(bb); it++) {
            Instruction* preInst = &((*it)->back());
            if (not isa<llvm::AllocaInst>(*preInst)) {
                predecessor.push_back(preInst);
            }
        }
    } else {
        for (auto it = bb->getInstList().rbegin(); it != bb->getInstList().rend(); it++) {
            Instruction* curInst = &(*it);
            if (inst == curInst) {
                it++;
                Instruction* preInst = &(*it);
                if (not isa<llvm::AllocaInst>(*preInst)) {
                    predecessor.push_back(preInst);
                }
            }
        }
    }

    return predecessor;
}

/*
 * Transfer function for instruction
 */
BitVector LiveVariableInLoop::transferFunction(Instruction *inst, BitVector &bv) {
    BitVectorBase KillBase, GenBase;

    if (llvm::isa<llvm::StoreInst>(*inst)) {
        auto op = inst->op_begin();
        if (varIndex.count(op->get()->getName()) > 0) {
            GenBase[op->get()->getName()] = varIndex[op->get()->getName()];
        }
        op++;
        if (varIndex.count(op->get()->getName()) > 0) {
            KillBase[op->get()->getName()] = varIndex[op->get()->getName()];
        }
    } else if (llvm::isa<llvm::LoadInst>(*inst)) {
        auto op = inst->op_begin();
        if (varIndex.count(inst->getName()) > 0) {
            KillBase[inst->getName()] = varIndex[inst->getName()];
        }
        if (varIndex.count(op->get()->getName()) > 0) {
            GenBase[op->get()->getName()] = varIndex[op->get()->getName()];
        }
    }

    BitVector kill = constructBitVector(KillBase);
    BitVector gen = constructBitVector(GenBase);
    BitVector newBv = LVAWorkList.transferFunction(bv, kill, gen);

    return newBv;
}

/*
 * Construct liveness info at each line(the Alloca instruction is filtered)
 */
void LiveVariableInLoop::getLineLivenessInfo() {
    for (auto it : LVAWorkList.instPreFactMap) {
        Instruction* inst = it.first;
        BitVector bv = it.second;
        if (llvm::isa<llvm::StoreInst>(*inst) or llvm::isa<llvm::LoadInst>(*inst)) {
            int instLine = inst->getDebugLoc().getLine();
            if (lineInfo.find(instLine) == lineInfo.end()) {
                lineInfo[instLine] = bv;
            } else {
                //error prone if the assumption does not hold
                lineInfo[instLine] |= bv;
            }
        }
    }
}

/*
 * Print the result to the console and testoutput.txt
 */
void LiveVariableInLoop::printLiveVariableInLoopResult(StringRef FuncName) {
    errs() << "================================================="
           << "\n";
    errs() << "LLVM-TUTOR: Live Variable results for `" << FuncName
           << "`\n";
    errs() << "=================================================\n";

    ofstream fout("testoutput.txt");
    map<int, string> bvIndex2varName;

    for (auto it = varIndex.begin(); it != varIndex.end(); it++) {
        bvIndex2varName[it->second] = it->first;
    }

    for (auto line_it = lineInfo.begin(); line_it != lineInfo.end(); line_it++) {
        errs() << "line " << line_it->first << ": {";
        fout << "line " << line_it->first << ": {";

        BitVector bv = line_it->second;
        for (int i = 0; i < bv.size(); i++) {
            if (bv.test(i)) {                          //i-th bit is 1
                string valueName = bvIndex2varName[i];
                errs() << valueName << " ";
                fout << valueName << " ";
            }
        }
        errs() << "}" << "\n";
        fout << "}" << "\n";
    }

    errs() << "-------------------------------------------------" << "\n\n";
}

/*
 * Debug Helper: Dump the pre-state(live variable name) of instruction
 */
void LiveVariableInLoop::dumpInstPreFactMap() {
    map<int, string> bvIndex2varName;

    for (auto it = varIndex.begin(); it != varIndex.end(); it++) {
        bvIndex2varName[it->second] = it->first;
    }

    for (auto it : LVAWorkList.instPreFactMap) {
        errs() << "Instruction: " << "\n";
        it.first->print(errs());
        errs() << "\n";

        for (int i = 0; i < it.second.size(); i++) {
            if (it.second.test(i)) {
                string valueName = bvIndex2varName[i];
                errs() << valueName << " ";
            }
        }
        errs() << "\n" << "\n";
    }
}

/*
 * Debug Helper: Print BitVector
 */
void LiveVariableInLoop::printBV(BitVector& bv) {
    for (int i = 0; i < bv.size(); i++) {
        if (bv.test(i)) {
            errs() << "1";
        } else {
            errs() << "0";
        }
    }
    errs() << "\n";
}

/*
 * This method tells LLVM which other passes we need to execute properly
 */
void LiveVariableInLoop::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<VariableInBB>();
    AU.setPreservesAll();
}

static RegisterPass<LiveVariableInLoop> X("live-var-intra", "LiveVariableInLoop Pass",
                                            true, // This pass doesn't modify the CFG => true
                                            false // This pass is not a pure analysis pass => false
);

static llvm::RegisterStandardPasses
        registerBBinLoopCounterPass(PassManagerBuilder::EP_EarlyAsPossible,
                                    [](const PassManagerBuilder &Builder,
                                       legacy::PassManagerBase &PM) {
                                        PM.add(new VariableInBB());
                                        PM.add(new LiveVariableInLoop());
                                    });