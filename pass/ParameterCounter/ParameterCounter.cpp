//
// Created by Sunshine on 13/4/2020.
//

#include <fstream>
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Argument.h"
#include "pass/ParameterCounter.h"

using namespace llvm;
using namespace std;

char ParameterCounter::ID = 0;

//----------------------------------------------------------
// ParameterCounter implementation
//----------------------------------------------------------

//This method tells LLVM which other passes we need to execute properly
void ParameterCounter::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

/*
 * Count the parameters in the function
 */
bool ParameterCounter::parseFunctionPara(llvm::Function &F) {
    int func_paraNum = 0;
    int func_doubleParaNum = 0;



    for (auto arg_it = F.arg_begin(); arg_it != F.arg_end(); arg_it++) {
        func_paraNum++;
        Type* paraType = arg_it->getType();
        if (paraType->isFloatTy()) {
            func_doubleParaNum++;
        }
    }

    paraNum.insert(pair<StringRef, int>(F.getName(), func_paraNum));
    floatParaNum.insert(pair<StringRef, int>(F.getName(), func_doubleParaNum));

    return false;
}


/*
 * Count the parameters in the module
 */
bool ParameterCounter::runOnModule(llvm::Module &M) {
    for (auto& F : M.getFunctionList()) {
        if (F.isDeclaration()) continue;
        parseFunctionPara(F);
    }
    printParameterCounterResult();

    return false;
}

/*
 * Print the result of parameter counter of module to the file
 */
void ParameterCounter::printParameterCounterResult() {
    ofstream fout("output.txt");
    for (auto it = paraNum.begin(); it != paraNum.end(); it++) {
        errs() << it->first.str() << "\t" << it->second << "\t" << floatParaNum[it->first] << "\n";
        fout << it->first.str() << "\t" << it->second << "\t" << floatParaNum[it->first] << "\n";
    }
}

static RegisterPass<ParameterCounter> X("parameter-counter", "ParameterCounter Pass",
                                       true, // This pass doesn't modify the CFG => true
                                       false // This pass is not a pure analysis pass => false
);
