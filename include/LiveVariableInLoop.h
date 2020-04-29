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
    typedef std::map<string, int> BitVectorMap;
    typedef std::queue<BitVectorMap> BitVectorList;

    //Record liveness of variables in each basic block
    struct SingleBasicBlockLivenessInfo {
        BitVectorList BBLiveVec;

        //Default constructor
        SingleBasicBlockLivenessInfo() {}

        //Ininitialize by inserting the bit vector of the last statement
        SingleBasicBlockLivenessInfo(BitVectorMap bv);

        //Add the liveness info of current statement to BBLiveVec
        void pushBitVector(BitVectorMap bv);

        //Get the liveness of first statement
        BitVectorMap getBasicHeadLiveInfo();

        //Get the liveness of last statement
        BitVectorMap getBasicTailLiveInfo();
    };

    //Record the calculation status of basic block
    struct BasicBlockWorkList {
        deque<BasicBlock*> BasicBlockList;
        map<BasicBlock*, int> state;   //state: 0 for non-calculated, 1 for calculated
        map<BasicBlock*, BitVectorMap> tailBVMap;   //bitvector of the last statement in Basic Block

        BasicBlockWorkList(Function &F);

        //Get Basic Block Status: 0 for non-calculated, 1 for calculated
        int getBasicBlockStatus(BasicBlock* bb);

        //Add and delete
        void pushBasicBlockToWorkList(BasicBlock* bb);
        void popBasicBlockToWorkList();

        //Merge two bitvectors
        BitVectorMap mergeBitVector(BitVectorMap &bv1, BitVectorMap &bv2, bool approx_para=true);

        //Merge the bitvectors at the entry of the basic block, propagated from the successors backward
        void mergeTailBVMap(BasicBlock* bb, BitVectorMap &bv, bool approx_para=true);

        //Judge whether the worklist is empty or not
        bool isEmpty();
    };

    struct LiveVariableInLoop : public llvm::FunctionPass {
        static char ID;
        BitVectorBase vectorBase;
        map<BasicBlock*, SingleBasicBlockLivenessInfo> BasicBlockLivenessInfo;
        map<int, BitVectorMap> lineInfo;


        LiveVariableInLoop() : llvm::FunctionPass(ID) {}
        void getAnalysisUsage(AnalysisUsage &AU) const;
        bool runOnFunction(Function &F) override;

        //Judege the equal relation of two bit vector
        bool isEqualBitVector(BitVectorMap &bv1, BitVectorMap &bv2);

        //Judge whether the fixed point has been reached
        bool hasReachedFixedPoint(BasicBlock* bb, BitVectorMap &connectBV);

        //Get the liveness info at each location in a single basic block and store the liveness info in the map
        BitVectorMap getLivenessInSingleBB(BasicBlock* bb, BitVectorMap &bv);

        //Get live variable in the last Basic Block by invoking getLivenessInSingleBB
        BitVectorMap getLivenessInLastBB(Function &F);

        //Get empty bit vector storing zero vector
        BitVectorMap generateEmptyBitVector();

        //Update the liveness by Kill and Gen set
        BitVectorMap transferFunction(BitVectorMap &bv, BitVectorBase &KillBase, BitVectorBase &GenBase);

        //Print the result
        void printLiveVariableInLoopResult(StringRef FuncName);
    };
}

#endif //HELLO_TRANSFORMATION_LIVEVARIABLEINLOOP_H
