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
    typedef std::map<string, int> BitVectorMap;
    typedef std::queue<BitVectorMap> BitVectorList;


    struct LiveVariableInBranch : public llvm::FunctionPass {
        static char ID;

        BitVectorBase vectorBase;
        map<BasicBlock*, SingleBasicBlockLivenessInfo> BasicBlockLivenessInfo;
        map<int, BitVectorMap> lineInfo;

        LiveVariableInBranch() : llvm::FunctionPass(ID) {}
        void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        bool runOnFunction(llvm::Function &F) override;

        // Judege the equal relation of two bit vector
        bool isEqualBitVector(BitVectorMap, BitVectorMap);

        // Judge whether the fixed point has been reached
        bool hasReachedFixedPoint(BasicBlock*, BitVectorMap);

        // Get the liveness info at each location in a single basic block and store the liveness info in the map
        BitVectorMap getLivenessInSingleBB(BasicBlock*, BitVectorMap);

        // Update the liveness
        BitVectorMap transferFunction(BitVectorMap, BitVectorBase, BitVectorBase);

        // Print the result
        void printLiveVariableInBranchResult(llvm::StringRef FuncName);
    };
}

#endif //TUTORIALPASS_LIVEVARIABLEINBRANCH_H
