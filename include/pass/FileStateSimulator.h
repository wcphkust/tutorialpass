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
    struct FileObj {
        Instruction* source;

        FileObj(AllocaInst* pSource) {
            source = pSource;
        }

        bool operator < (const FileObj & b) const {
            return (source < b.source);
        }
    };

    enum FileState{
        INIT,
        OPEN,
        CLOSE,
        ERROR
    };


    struct PathFileState{
        vector<BasicBlock*> path;
        FileState state;
    };

    // intraprocedural, path sensitive
    // no loop in the function
    struct FileStateSimulator : public llvm::FunctionPass {
        static char ID;
        set<FileObj> fileObjSet;
        map<string, set<string>> pointToSet;
        map<FileObj, set<PathFileState>> collector;
        map<FileObj*, set<string>> aliaset;

        FileStateSimulator() : llvm::FunctionPass(ID) {}

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
        bool runOnFunction(llvm::Function &F) override;

        void transferFunction(BasicBlock* bb);
        void printFileStateSimulatorResult(llvm::StringRef FuncName);
    };
}


#endif //HELLO_TRANSFORMATION_FILESTATESIMULATOR_H
