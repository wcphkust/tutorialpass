//
// Created by Sunshine on 11/3/2020.
//

#ifndef LLVMLAB_CONDCOLLECTOR_H
#define LLVMLAB_CONDCOLLECTOR_H

//========================================================================
// FILE:
//    CondCollector.h
//
// DESCRIPTION:
//    Declares the CondCollector Pass
//    Counts the number of branch condition related to the container
//
// License: MIT
//========================================================================

#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace llvm;

//------------------------------------------------------------------------------
// Legacy PM interface
//------------------------------------------------------------------------------

struct CondCollector : public ModulePass {
    static char ID;
    map<Function*, vector<Value*>> containerBranchCondList;

    CondCollector() : ModulePass(ID) {}
    virtual bool runOnModule(Module &M);
    bool runOnFunction(Function &F);

    bool checkConditionForm(Value* cond);
    bool checkCondOperandForm(Value* value);

    void getAnalysisUsage(AnalysisUsage &AU) const;
    void printCondCollector(Module &M);
};

#endif