//
// Created by chasen on 29/4/20.
//

#ifndef HELLO_TRANSFORMATION_LIVEVARIABLEINLOOP_H
#define HELLO_TRANSFORMATION_LIVEVARIABLEINLOOP_H


#include <utility>
#include <queue>
#include <map>
#include <vector>
#include <set>
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace std;
using namespace llvm;

//------------------------------------------------------------------------------
// PM interface
//------------------------------------------------------------------------------

namespace {
    typedef map<string, unsigned> BitVectorBase;
    typedef deque<Instruction*> (*InstDepsFunc)(Instruction*); //deps for instruction, error prone

    struct InstWorkList {
        deque<Instruction*> workList;   //TO BE polished
        BitVectorBase varIndex;
        map<Instruction*, BitVector> instFactMap;

        InstWorkList() = default;
        InstWorkList(Function &F, bool isForward);

        BitVector transferFunction(BitVector &bv, BitVectorBase &KillBase, BitVectorBase &GenBase);

        //Add and delete
        void pushInstToWorkList(Instruction* inst);
        void pushDepsInstToWorkList(Instruction* inst, InstDepsFunc deps);
        void popInstToWorkList();
        Instruction* getWorkListHead();

        void insertBitVector(Instruction* inst, BitVector& bv);

        //Judge whether the worklist is empty or not
        bool isEmpty();
    };

    struct LiveVariableInLoop : public llvm::FunctionPass {
        static char ID;
        BitVectorBase varIndex;
        map<int, BitVector> lineInfo;
        InstWorkList LVAWorkList;

        LiveVariableInLoop() : llvm::FunctionPass(ID) {}

        bool runOnFunction(Function &F) override;

        //Deps function for instruction in LVA
        static deque<Instruction*> predDepsFunc(Instruction* inst);

        //Update the liveness by Kill and Gen set
        bool transferFunction(Instruction* inst, BitVector& bv);

        //construct liveness info at each line
        void getLineLivenessInfo();

        //Print the result
        void printLiveVariableInLoopResult(StringRef FuncName);

        void getAnalysisUsage(AnalysisUsage &AU) const;
    };
}

#endif //HELLO_TRANSFORMATION_LIVEVARIABLEINLOOP_H
