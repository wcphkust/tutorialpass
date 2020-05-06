//
// Created by chasen on 29/4/20.
//

#ifndef HELLO_TRANSFORMATION_LIVEVARIABLEVIAINST_H
#define HELLO_TRANSFORMATION_LIVEVARIABLEVIAINST_H


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
#include "util/WorkList.h"

using namespace std;
using namespace llvm;
using namespace PassUtilSpace;

//------------------------------------------------------------------------------
// Live Variable Analysis by Instruction
//------------------------------------------------------------------------------

namespace {
    struct LiveVariableViaInst : public llvm::FunctionPass {
        static char ID;
        BitVectorBase varIndex;
        map<int, BitVector> lineInfo;
//        WorkList<Instruction> LVAWorkList;
        InstWorkList LVAWorkList;

        LiveVariableViaInst() : llvm::FunctionPass(ID) {}

        bool runOnFunction(Function &F);
        void getAnalysisUsage(AnalysisUsage &AU);

        //Core instantiations
        static deque<Instruction*> predDepsFunc(Instruction* inst);  //Deps function for instruction in LVA
        BitVector constructBitVector(BitVectorBase &base);  //construct bitvector from the set of variables
        BitVector transferFunction(Instruction* inst, BitVector& bv);  //Update the liveness by Kill and Gen set

        //Print the result
        void getLineLivenessInfo();  //construct liveness info at each line
        void printLiveVariableInLoopResult(StringRef FuncName);  //Print the result

        //Debug helper function
        void printBV(BitVector& bv);
        void dumpInstPreFactMap();
    };
}

#endif
