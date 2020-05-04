//
// Created by Sunshine on 13/3/2020.
//

#ifndef TUTORIALPASS_BBCOUNTER_H
#define TUTORIALPASS_BBCOUNTER_H

//========================================================================
// FILE:
//    BBCounter.h
//
// DESCRIPTION:
//    Declares the BBCounter Pass
//    Count the basic blocks in the loop
//
// License: MIT
//========================================================================

#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using ResultBBCounter = unsigned;

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct BBCounter : public llvm::PassInfoMixin<BBCounter> {
    using Result = ResultBBCounter;
    llvm::PreservedAnalyses run(llvm::Function &F,
                                llvm::FunctionAnalysisManager &);
    BBCounter::Result generateBBCount(llvm::Function &F);
};

//------------------------------------------------------------------------------
// Legacy PM interface
//------------------------------------------------------------------------------
struct LegacyBBCounter : public llvm::FunctionPass {
    static char ID;
    LegacyBBCounter() : llvm::FunctionPass(ID) {}
    bool runOnFunction(llvm::Function &F);

    BBCounter Impl;
};

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
// Pretty-prints the result of this analysis
void printBBCounterResult(const ResultBBCounter &counter,
                              llvm::StringRef FuncName);

#endif //TUTORIALPASS_BBCOUNTER_H
