//
// Created by Sunshine on 16/3/2020.
// Reference: https://stackoverflow.com/questions/47978363/get-variable-name-in-llvm-pass
//

#include <set>
#include <stack>
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "LiveVariableInBB.h"
#include "VariableInBB.h"

using namespace std;
using namespace llvm;

char LiveVariableInBB::ID = 0;

//----------------------------------------------------------
// BBinLoopCounter implementation
//----------------------------------------------------------

//This method tells LLVM which other passes we need to execute properly
void LiveVariableInBB::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<VariableInBB>();
    AU.setPreservesAll();
}

/*
 * Count the basic blocks in the loops
 */
bool LiveVariableInBB::runOnFunction(llvm::Function &F) {
    set<string> varSet = getAnalysis<VariableInBB>().getVarSet();

    //construct the BitVectorBase
    for (set<string>::iterator it = varSet.begin(); it != varSet.end(); ++it) {
        vectorBase.push_back(*it);
    }

    //start the main process of LiveVariableInBB pass
    LiveInfo.push(generateEmptyBitVector());
    bool flag = true;
    unsigned preLine = 0;
    BitVectorBase KillBase, GenBase;

    //DEBUG
//    unsigned currentLine = 0;

    for (auto bb = F.getBasicBlockList().rbegin(); bb != F.getBasicBlockList().rend(); bb++){
        for (auto inst = bb->getInstList().rbegin(); inst != bb->getInstList().rend(); inst++) {
            //skip AllocaInst
            if (llvm::isa<llvm::AllocaInst>(*inst)) {
                continue;
            }

            errs() << *inst << "\n";
            unsigned currentLine = inst->getDebugLoc().getLine();
            errs() << "currentLine" << currentLine << "\n";
            if (flag) {
                preLine = currentLine;
                flag = false;
            }
            if (currentLine != preLine) {
                errs() << "LiveInfo size " << LiveInfo.size() << "\n";
                BitVectorMap bv = transferFunction(LiveInfo.top(), KillBase, GenBase);
                KillBase.clear();
                GenBase.clear();
                LiveInfo.push(bv);
                LocInfo.push(preLine);
                preLine = currentLine;
            }
            //Store
            if (llvm::isa<llvm::StoreInst>(*inst)) {
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
            }
            //Load
            if (llvm::isa<llvm::LoadInst>(*inst)) {
                errs() << "-----------------------------" << "\n";
                errs() << "Load instruction" << "\n";
                errs() << (*inst) << "\n";
                auto op = inst->op_begin();
                errs() << "Kill:" << inst->getName() << "\n";
                KillBase.push_back(inst->getName());
                errs() << "Gen:" << op->get()->getName() << "\n";
                GenBase.push_back(op->get()->getName());
                errs() << "-----------------------------" << "\n";
            }
        }
    }
    printLiveVariableInBBResult(F.getName());
    return false;
}

BitVectorMap LiveVariableInBB::generateEmptyBitVector() {
    BitVectorMap bv;
    for (int i = 0; i < vectorBase.size(); i++) {
        bv[vectorBase[i]] = 0;
    }
    return bv;
}

BitVectorMap LiveVariableInBB::transferFunction(BitVectorMap bv, BitVectorBase KillBase, BitVectorBase GenBase) {
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

void LiveVariableInBB::printLiveVariableInBBResult(StringRef FuncName) {
    errs() << "================================================="
           << "\n";
    errs() << "LLVM-TUTOR: Live Variable results for `" << FuncName
           << "`\n";
    errs() << "=================================================\n";

    while(!LiveInfo.empty() and !LocInfo.empty()) {
        BitVectorMap bv = LiveInfo.top();
        unsigned line = LocInfo.top();
        errs() << line << ": {";
        for (BitVectorMap::iterator it2 = bv.begin(); it2 != bv.end(); it2++) {
            if (it2->second) {
                errs() << it2->first << "  ";
            }
        }
        errs() << "}" << "\n";
        LiveInfo.pop();
        LocInfo.pop();
    }
    errs() << "-------------------------------------------------" << "\n\n";
}

static RegisterPass<LiveVariableInBB> X("live-var-in-bb", "LiveVariableInBB Pass",
                                       true, // This pass doesn't modify the CFG => true
                                       false // This pass is not a pure analysis pass => false
);

static llvm::RegisterStandardPasses
        registerBBinLoopCounterPass(PassManagerBuilder::EP_EarlyAsPossible,
                                    [](const PassManagerBuilder &Builder,
                                       legacy::PassManagerBase &PM) {
                                        PM.add(new VariableInBB());
                                        PM.add(new LiveVariableInBB());
                                    });