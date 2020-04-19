//
// Created by Sunshine on 16/3/2020.
//

#ifndef TUTORIALPASS_VARIABLEINBB_H
#define TUTORIALPASS_VARIABLEINBB_H

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include <string>
#include <set>

using namespace std;

struct VariableInBB : public llvm::FunctionPass {
    static char ID;
    set<string> varSet;

    VariableInBB() : llvm::FunctionPass(ID) {}
    bool runOnFunction(llvm::Function &F) override;
    set<string> getVarSet();
};

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
// Pretty-prints the result of this analysis
void printVariableInBBResult(const set<string> varSet, llvm::StringRef FuncName);

#endif //TUTORIALPASS_VARIABLEINBB_H
