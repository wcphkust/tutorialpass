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

    enum AnalyzeDirection {
        FORWARD,
        BACKWORD
    };

    enum ApproximationMode {
        MAY,
        MUST
    };

    struct InstWorkList {
        deque<Instruction*> workList;   //TO BE polished
        map<Instruction*, BitVector> instPreFactMap;    //pre-state of an instruction
        map<Instruction*, BitVector> instPostFactMap;   //post-state of an instruction
        AnalyzeDirection direction;
        ApproximationMode approxMode;

        InstWorkList() = default;
        InstWorkList(Function &F, BitVectorBase pVarIndex, bool isForward = true, bool isMay = true);

        //Operations on worklist
        bool pushInstToWorkList(Instruction* inst);
        void pushDepsInstToWorkList(Instruction* inst, InstDepsFunc deps);
        void popInstToWorkList();
        bool isEmpty();
        Instruction* getWorkListHead();

        //Operations on pre/post fact map
        void insertPreBitVector(Instruction* inst, BitVector bv);
        void insertPostBitVector(Instruction* inst, BitVector bv);

        //Operations on bit vectors
        BitVector transferFunction(BitVector &bv, BitVector &Kill, BitVector &Gen);  //Kill-Gen transfer function
        BitVector join(BitVector& bv1, BitVector& bv2);
        bool isFixedPoint(BitVector bv, Instruction* inst);    //judge fixed-point by pre-state of an instruction
    };

    struct LiveVariableInLoop : public llvm::FunctionPass {
        static char ID;
        BitVectorBase varIndex;
        map<int, BitVector> lineInfo;
        InstWorkList LVAWorkList;

        LiveVariableInLoop() : llvm::FunctionPass(ID) {}

        bool runOnFunction(Function &F) override;
        void getAnalysisUsage(AnalysisUsage &AU) const;

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
