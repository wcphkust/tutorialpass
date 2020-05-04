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
#include "pass/LiveVariableViaInst.h"
#include "pass/VariableInBB.h"

using namespace std;
using namespace llvm;
using namespace PassUtilSpace;

char LiveVariableViaInst::ID = 0;

//----------------------------------------------------------
// Implementation of LiveVariableViaInst
//---------------------------------------------------------—

/*
 * Main function of Live Variable Analysis in Branch, and can be generalized to loop
 */
bool LiveVariableViaInst::runOnFunction(llvm::Function &F) {
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
deque<Instruction*> LiveVariableViaInst::predDepsFunc(Instruction* inst) {
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
 * Construct BitVector from BitVectorBase(the set of the fact)
 */
BitVector LiveVariableViaInst::constructBitVector(BitVectorBase &base) {
    BitVector bv(varIndex.size(), false);

    for (auto it = base.begin(); it != base.end(); it++) {
        if (varIndex.find(it->first) != varIndex.end()) {
            bv.set(varIndex[it->first]);
        }
    }
    return bv;
}

/*
 * Transfer function for instruction
 */
BitVector LiveVariableViaInst::transferFunction(Instruction *inst, BitVector &bv) {
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
void LiveVariableViaInst::getLineLivenessInfo() {
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
void LiveVariableViaInst::printLiveVariableInLoopResult(StringRef FuncName) {
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
void LiveVariableViaInst::dumpInstPreFactMap() {
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
void LiveVariableViaInst::printBV(BitVector& bv) {
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
void LiveVariableViaInst::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<VariableInBB>();
    AU.setPreservesAll();
}

static RegisterPass<LiveVariableViaInst> X("live-var-via-inst", "LiveVariableViaInst Pass",
                                           true, // This pass doesn't modify the CFG => true
                                            false // This pass is not a pure analysis pass => false
);

static llvm::RegisterStandardPasses
        registerBBinLoopCounterPass(PassManagerBuilder::EP_EarlyAsPossible,
                                    [](const PassManagerBuilder &Builder,
                                       legacy::PassManagerBase &PM) {
                                        PM.add(new VariableInBB());
                                        PM.add(new LiveVariableViaInst());
                                    });