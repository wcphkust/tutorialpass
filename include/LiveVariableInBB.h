//
// Created by Sunshine on 16/3/2020.
//

#ifndef TUTORIALPASS_LIVEVARIABLEINBB_H
#define TUTORIALPASS_LIVEVARIABLEINBB_H

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace std;

//------------------------------------------------------------------------------
// PM interface
//------------------------------------------------------------------------------

namespace {
    typedef std::vector<string> BitVectorBase;
    typedef std::map<string, int> BitVector;
    typedef std::stack<BitVector> BitVectorList;

    struct LiveVariableInBB : public llvm::FunctionPass {
        static char ID;
        BitVectorBase vectorBase;
        BitVectorList LiveInfo;
        stack<unsigned> LocInfo;

        LiveVariableInBB() : llvm::FunctionPass(ID) {}
        void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        bool runOnFunction(llvm::Function &F) override;

        BitVector generateEmptyBitVector();
        BitVector transferFunction(BitVector bv, BitVectorBase KillBase, BitVectorBase GenBase);
        void printLiveVariableInBBResult(llvm::StringRef FuncName);
    };
}
#endif //TUTORIALPASS_LIVEVARIABLEINBB_H
