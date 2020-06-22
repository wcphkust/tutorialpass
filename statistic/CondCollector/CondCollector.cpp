//
// Created by chasen on 21/6/20.
//

#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/GlobalValue.h"
#include "statistic/CondCollector.h"
#include "util/demangle.h"

char CondCollector::ID = 0;

bool CondCollector::runOnModule(Module &M) {
    for (auto& F : M) {
        if (F.getLinkage()==GlobalValue::LinkOnceAnyLinkage || F.getLinkage()==GlobalValue::LinkOnceODRLinkage) {
            continue;
        }
        runOnFunction(F);
    }

    printCondCollector(M);
    return false;
}


bool CondCollector::runOnFunction(llvm::Function &F) {
    for (auto& BB : F) {
        for (auto& inst : BB) {
            if (auto* brInst = dyn_cast<BranchInst>(&inst)) {
                Value* condition = (brInst->isConditional() ? brInst->getCondition() : NULL);

                if (checkConditionForm(condition)) {
                    containerBranchCondList[&F].push_back(condition);
                }
            }
        }
    }

    return false;
}


bool CondCollector::checkConditionForm(Value *value) {
    if (value == NULL) {
        return false;
    }

    //consider function call
    if (auto* funcInst = dyn_cast<CallInst>(value)) {
        //todo
    }

    if (auto* icmpInst = dyn_cast<ICmpInst>(value)) {
        Value* op1 = icmpInst->getOperand(0);
        Value* op2 = icmpInst->getOperand(1);

        if (checkCondOperandForm(op2) || checkCondOperandForm(op1)) {
            return true;
        }
    }

    return false;
}


bool CondCollector::checkCondOperandForm(Value *value) {
    if (auto* callInst = dyn_cast<CallInst>(value)) {
        string normalizedStr = cxx_demangle(callInst->getFunction()->getName().str());
        errs() << "OK" << "\n";
        errs() << normalizedStr << "\n";

        if (normalizedStr.find("std::vector<int, std::allocator<int> >")) {
            return true;
        }
    }
    return false;
}



void CondCollector::printCondCollector(Module &M) {
    errs() << "================================================="
                 << "\n";
    errs() << "LLVM-TUTOR: Condition Collector results for `" << M.getName()
                 << "`\n";
    errs() << "=================================================\n";
    const char *str2 = "Container Branch Count";
    const char *str1 = "Function Name";
    errs() << format("%-20s %-10s\n", str1, str2);
    errs() << "-------------------------------------------------"
                 << "\n";

    for (auto it : containerBranchCondList) {
        string funcName = cxx_demangle(it.first->getName().str());
        errs() << format("%-20s %-10lu\n", funcName.c_str(), containerBranchCondList[it.first].size());
    }

    errs() << "-------------------------------------------------"
           << "\n\n";
}


static RegisterPass<CondCollector> X("condcollector", "Condition Collector Pass", true, false);

void CondCollector::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesAll();
}