//
// Created by chasen on 6/5/20.
//

#ifndef HELLO_TRANSFORMATION_FILESTATESIMULATOR_H
#define HELLO_TRANSFORMATION_FILESTATESIMULATOR_H

#include <iostream>
#include <fstream>
#include <set>
#include <stack>
#include <map>
#include <vector>
#include "llvm/IR/CFG.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"

using namespace llvm;
using namespace std;

namespace {
    typedef vector<BasicBlock*> Path;
    typedef map<string, string> FileStates;

    // intraprocedural, path sensitive
    // no loop in the function
    struct FileStateSimulator : public llvm::FunctionPass {
        static char ID;
        Function* func;
        vector<Path> allPathArray;
        map<Path*, FileStates> pathFileStates;
        map<Value*, Use*> defUseMap;

        FileStateSimulator() : llvm::FunctionPass(ID) {}

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        bool runOnFunction(llvm::Function &F) override;

        void collectAllPath();
        void initializeAllPathState();

        void handleFunctionCall(CallInst* inst, Path* path);
        void computeFileStateOnePath(Path* path);
        void computeFileStateOneBasicBlock(BasicBlock* bb, Path* path);
        void printFileStateSimulatorResult();

        void extractDefUseMap(Function* F);
    };
}


#endif //HELLO_TRANSFORMATION_FILESTATESIMULATOR_H
