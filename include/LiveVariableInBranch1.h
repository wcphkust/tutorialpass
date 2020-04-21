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
    typedef std::vector<string> BitVectorBase;         /* List of variable names in functions*/
    typedef std::map<string, int> BitVectorMap;        /* Liveness info at a program location*/
    typedef std::queue<BitVectorMap> BitVectorList;    /* Liveness info of the program locations along a forward/backward path */

    //Record liveness of variables in each basic block
    struct SingleBasicBlockLivenessInfo {
        BitVectorList BBLiveVec;

        SingleBasicBlockLivenessInfo() {}

        //Ininitialize by inserting the bit vector of the last statement
        SingleBasicBlockLivenessInfo(BitVectorMap BV);

        //Add the liveness info of current statement to BBLiveVec
        void pushBitVector(BitVectorMap BV);

        //Get the liveness of first statement
        BitVectorMap getBasicHeadLiveInfo();

        //Get the liveness of last statement
        BitVectorMap getBasicTailLiveInfo();
    };

    struct LiveVariableInBranch : public llvm::FunctionPass {
        static char ID;

        /* Record the variable names in function */
        BitVectorBase vectorBase;

        /* Record the liveness information of basic block */
        map<BasicBlock *, SingleBasicBlockLivenessInfo> BasicBlockLivenessInfo;


        LiveVariableInBranch() : llvm::FunctionPass(ID) {}
        void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        bool runOnFunction(llvm::Function &F) override;


        /* Get the liveness info in a basic block and store it in the map */
        BitVectorMap getLivenessInSingleBB(BasicBlock &, BitVectorMap);

        // Update the liveness
        void transferFunction(BitVectorMap &, BitVectorBase &, BitVectorBase &);

        /* Print the result */
        void printLiveVariableInBranchResult(llvm::StringRef FuncName);
    };
}

#endif //TUTORIALPASS_LIVEVARIABLEINBRANCH_H
