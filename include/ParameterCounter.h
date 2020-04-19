//
// Created by Sunshine on 13/4/2020.
//

#ifndef TUTORIALPASS_PARAMETERCOUNTER_H
#define TUTORIALPASS_PARAMETERCOUNTER_H

//========================================================================
// FILE:
//    ParameterCounter.h
//
// DESCRIPTION:
//    Declares the Parameter Counter Pass
//    Count the parameters in the function
//
// License: MIT
//========================================================================

#include <map>
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

struct ParameterCounter : public llvm::ModulePass {
    static char ID;
    std::map<llvm::StringRef, int> paraNum;
    std::map<llvm::StringRef, int> floatParaNum;

    ParameterCounter() : llvm::ModulePass(ID) {}
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    bool parseFunctionPara(llvm::Function &F);
    bool runOnModule(llvm::Module &M) override;

    void printParameterCounterResult();
};

#endif //TUTORIALPASS_PARAMETERCOUNTER_H
