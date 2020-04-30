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
 * isForward: is forward-analysis
 *   true: initialize by adding the first instruction of entry
 *   false: initialize by adding the first instruction of entry
 */
InstWorkList::InstWorkList(Function &F, bool isForward = true) {
    if (isForward) {
        auto it = F.getBasicBlockList().begin();
        BasicBlock* entryBB = &(*it);
        Instruction* firstInst = &(entryBB->front());
        workList.push_back(firstInst);
    } else {
        auto it = F.getBasicBlockList().rbegin();
        BasicBlock* exitBB = &(*it);
        Instruction* lastInst = &(exitBB->back());
        workList.push_back(lastInst);
    }
}

BitVector InstWorkList::transferFunction(BitVector &bv, BitVectorBase &KillBase, BitVectorBase &GenBase) {
    BitVector kill(varIndex.size(), false);
    BitVector gen(varIndex.size(), false);
    for (auto it = varIndex.begin(); it != varIndex.end(); it++) {
        if (KillBase.find(it->first) != KillBase.end()) {
            kill.set(varIndex[it->first]);
        }
        if (GenBase.find(it->first) != GenBase.end()) {
            gen.set(varIndex[it->first]);
        }
    }
    BitVector resBv(bv);
    resBv &=  kill.flip();
    resBv |= gen;
    return resBv;
}

/*
 * Add instruction to worklist
 */
void InstWorkList::pushInstToWorkList(Instruction *inst) {
    bool isInWorkList = false;
    for (auto & it : workList) {
        if (inst == it) {
            isInWorkList = true;
        }
    }

    if (not isInWorkList) {
        workList.push_back(inst);
    }
}

/*
 * Add the instructions which depend on inst
 */
void InstWorkList::pushDepsInstToWorkList(Instruction *inst, InstDepsFunc deps) {
    deque<Instruction*> depInsts = deps(inst);
    for (auto & depInst : depInsts) {
        pushInstToWorkList(depInst);
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
 * Insert bit vector
 */
void InstWorkList::insertBitVector(Instruction* inst, BitVector &bv) {
    instFactMap[inst] = bv;
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
//----------------------------------------------------------

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

    errs() << "debug info 1" << "\n";
    errs() << index << "\n";

    //collect the instruction at the exit node and initialize the worklist
    LVAWorkList = InstWorkList(F, false);

    //Worklist algorithm
    int iterNum = 0;
    while (not LVAWorkList.isEmpty()) {
        iterNum++;
        Instruction* inst = LVAWorkList.getWorkListHead();
        BitVector bv = LVAWorkList.instFactMap[inst];
        if (transferFunction(inst, bv)) {
            LVAWorkList.pushDepsInstToWorkList(inst, predDepsFunc);  //TO BE reviewed
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
        for (auto it = pred_begin(bb); it != pred_begin(bb); it++) {
            Instruction* preInst = &((*it)->back());
            predecessor.push_back(preInst);
        }
    } else {
        for (auto it = bb->getInstList().rbegin(); it != bb->getInstList().rend(); it++) {
            Instruction* curInst = &(*it);
            if (inst == curInst) {
                it++;
                Instruction* preInst = &(*it);
                predecessor.push_back(preInst);
            }
        }
    }
    return predecessor;
}

/*
 * Get the liveness info at each location in a single instruction and store the liveness info in the map
 * Parameter:
 *  bb: the pointer of instruction
 *  bv: the initial liveness state, i.e. the liveness info of the last statement
 * Return: the liveness info the first statement
 */
bool LiveVariableInLoop::transferFunction(Instruction *inst, BitVector &bv) {
    BitVectorBase KillBase, GenBase;

    if (llvm::isa<llvm::StoreInst>(*inst)) {
        //Store
        errs() << "-----------------------------" << "\n";
        errs() << "Store instruction" << "\n";
        errs() << (*inst) << "\n";
        auto op = inst->op_begin();
        errs() << "Gen:" << op->get()->getName() << "\n";
        GenBase[op->get()->getName()] = varIndex[op->get()->getName()];
        op++;
        errs() << "Kill:" << op->get()->getName() << "\n";
        KillBase[op->get()->getName()] = varIndex[op->get()->getName()];
        errs() << "-----------------------------" << "\n";
    } else if (llvm::isa<llvm::LoadInst>(*inst)) {
        //Load
        errs() << "-----------------------------" << "\n";
        errs() << "Load instruction" << "\n";
        errs() << (*inst) << "\n";
        auto op = inst->op_begin();
        errs() << "Kill:" << inst->getName() << "\n";
        KillBase[inst->getName()] = varIndex[inst->getName()];
        errs() << "Gen:" << op->get()->getName() << "\n";
        GenBase[op->get()->getName()] = varIndex[op->get()->getName()];
        errs() << "-----------------------------" << "\n";
    }

    KillBase.clear();
    GenBase.clear();

    errs() << "line number: " << inst->getDebugLoc().getLine();

    BitVector newBv = LVAWorkList.transferFunction(bv, KillBase, GenBase);
    LVAWorkList.insertBitVector(inst, newBv);

    return (bv == newBv);
}


/*
 * Construct liveness info at each line
 * Assumption:
 *   inst1, inst2: two keys in instFactMap, correspond to the same line in source file
 *   The iteration order in getLineLivenessInfo consist with their order in CFG
 */
void LiveVariableInLoop::getLineLivenessInfo() {
    for (auto it = LVAWorkList.instFactMap.begin(); it != LVAWorkList.instFactMap.end(); it++) {
        Instruction* inst = it->first;
        BitVector bv = it->second;
        int instLine = inst->getDebugLoc().getLine();
        if (lineInfo.find(instLine) == lineInfo.end()) {
            lineInfo[instLine] = bv;
        } else {
            //error prone if the assumption does not hold
            lineInfo[instLine] = bv;
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

    auto line_it = lineInfo.begin();
    ofstream fout("testoutput.txt");

    map<int, string> bv2varName;
    for (auto it = varIndex.begin(); it != varIndex.end(); it++) {
        bv2varName[it->second] = it->first;
    }

    for ( ; line_it != lineInfo.end(); line_it++) {
        errs() << "line: " << line_it->first << " {";
        fout << "line:" << line_it->first << " {";

        BitVector bv = line_it->second;
        int varIndexInBVBase = 0;
        for (auto bit_it = bv.set_bits_begin(); bit_it != bv.set_bits_end(); bit_it++) {
            if (*bit_it == 1) {
                string valueName = bv2varName[varIndexInBVBase];
                errs() << " ";
                fout << " ";
            }
            varIndexInBVBase++;
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