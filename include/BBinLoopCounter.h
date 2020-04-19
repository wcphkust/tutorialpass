//
// Created by Sunshine on 14/3/2020.
//

#ifndef TUTORIALPASS_BBINLOOPCOUNTER_H
#define TUTORIALPASS_BBINLOOPCOUNTER_H

//
// Created by Sunshine on 13/3/2020.
//

//========================================================================
// FILE:
//    BBinLoopCounter.h
//
// DESCRIPTION:
//    Declares the BBinLoopCounter Pass
//    Count the basic blocks in the loop
//
// License: MIT
//========================================================================

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

//------------------------------------------------------------------------------
// PM interface
//------------------------------------------------------------------------------
namespace {
    struct BBinLoopCounter : public llvm::FunctionPass {
        static char ID;
        BBinLoopCounter() : llvm::FunctionPass(ID) {}
        void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        bool runOnFunction(llvm::Function &F) override;
    };
}


#endif //TUTORIALPASS_BBINLOOPCOUNTER_H
