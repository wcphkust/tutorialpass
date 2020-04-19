//
// Created by Sunshine on 26/3/2020.
//

#ifndef TUTORIALPASS_LIVEVARIABLEINBRANCH_H
#define TUTORIALPASS_LIVEVARIABLEINBRANCH_H

#include <utility>
#include <queue>
#include <map>
#include <vector>
#include <set>
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/ADT/StringMap.h"
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
    typedef std::vector<string> BitVectorBase;
    typedef std::map<string, int> BitVector;
    typedef std::queue<BitVector> BitVectorList;

    //Record liveness of variables in each basic block
    struct SingleBasicBlockLivenessInfo {
        BitVectorList BBLiveVec;

        //Default constructor
        SingleBasicBlockLivenessInfo() {}

        //Ininitialize by inserting the bit vector of the last statement
        SingleBasicBlockLivenessInfo(BitVector bv);

        //Add the liveness info of current statement to BBLiveVec
        void pushBitVector(BitVector bv);

        //Get the liveness of first statement
        BitVector getBasicHeadLiveInfo();

        //Get the liveness of last statement
        BitVector getBasicTailLiveInfo();
    };

    //Record the calculation status of basic block
    struct BasicBlockWorkList {
        deque<BasicBlock*> BasicBlockList;
        map<BasicBlock*, int> state;   //state: 0 for non-calculated, 1 for calculated
        map<BasicBlock*, BitVector> tailBVMap;   //bitvector of the last statement in Basic Block

        BasicBlockWorkList(Function &F);

        //Get Basic Block Status: 0 for non-calculated, 1 for calculated
        int getBasicBlockStatus(BasicBlock* bb);

        //Add and delete
        void pushBasicBlockToWorkList(BasicBlock* bb);
        void popBasicBlockToWorkList();

        //Merge two bitvectors
        BitVector mergeBitVector(BitVector bv1, BitVector bv2, bool approx_para=true);

        //Merge the bitvectors at the entry of the basic block, propagated from the successors backward
        void mergeTailBVMap(BasicBlock* bb, BitVector bv, bool approx_para=true);

        //Judge whether the worklist is empty or not
        bool isEmpty();
    };

    struct LiveVariableInBranch : public llvm::FunctionPass {
        static char ID;
        BitVectorBase vectorBase;
        map<BasicBlock*, SingleBasicBlockLivenessInfo> BasicBlockLivenessInfo;
        map<int, BitVector> lineInfo;


        LiveVariableInBranch() : llvm::FunctionPass(ID) {}
        void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        bool runOnFunction(llvm::Function &F) override;

        //Judege the equal relation of two bit vector
        bool isEqualBitVector(BitVector bv1, BitVector bv2);

        //Judge whether the fixed point has been reached
        bool hasReachedFixedPoint(BasicBlock* bb, BitVector connectBV);

        //Get the liveness info at each location in a single basic block and store the liveness info in the map
        BitVector getLivenessInSingleBB(BasicBlock* bb, BitVector bv);

        //Get live variable in the last Basic Block by invoking getLivenessInSingleBB
        BitVector getLivenessInLastBB(Function &F);

        //Get empty bit vector storing zero vector
        BitVector generateEmptyBitVector();

        //Update the liveness by Kill and Gen set
        BitVector transferFunction(BitVector bv, BitVectorBase KillBase, BitVectorBase GenBase);

        //Print the result
        void printLiveVariableInBranchResult(llvm::StringRef FuncName);
    };
}

#endif //TUTORIALPASS_LIVEVARIABLEINBRANCH_H
