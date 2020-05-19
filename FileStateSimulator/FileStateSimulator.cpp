//
// Created by chasen on 6/5/20.
//

#include <set>
#include "llvm/IR/CFG.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "pass/LiveVariableViaInst.h"
#include "FileStateSimulator.h"

using namespace std;
using namespace llvm;

char FileStateSimulator::ID = 0;

//----------------------------------------------------------
// Implementation of FileStateSimulator
//---------------------------------------------------------—

/*
 * This method tells LLVM which other passes we need to execute properly
 */
void FileStateSimulator::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

/*
 * Main function of FileStateSimulator in function, which does not consider the loop
 */
bool FileStateSimulator::runOnFunction(llvm::Function &F) {
    func = &F;

    collectAllPath();
    initializeAllPathState();

    for (auto & p : allPathArray) {
        Path* path = &p;
        computeFileStateOnePath(path);
    }

    printFileStateSimulatorResult();
}


void FileStateSimulator::collectAllPath() {
    BasicBlock* bb = &func->getEntryBlock();
    vector<Path> searchPathVec;
    Path initialPath;

    //initialize the path set
    initialPath.push_back(bb);
    searchPathVec.push_back(initialPath);

    //breath first search
    while(!searchPathVec.empty()) {
        Path singlePath = searchPathVec[searchPathVec.size() - 1];
        BasicBlock* lastBasicBlock = singlePath[singlePath.size() - 1];
        searchPathVec.pop_back();

        for (auto it = succ_begin(lastBasicBlock); it != succ_end(lastBasicBlock); it++) {
            Path augmentingPath = singlePath;
            augmentingPath.push_back(*it);
            if (succ_begin(*it) == succ_end(*it)) {
                allPathArray.push_back(augmentingPath);   //*it has no successor
            } else {
                searchPathVec.push_back(augmentingPath);
            }
        }
    }
}

//This function can be omitted
void FileStateSimulator::initializeAllPathState() {
    for (auto path : allPathArray) {
        FileStates emptyState;
        pathFileStates[&path] = emptyState;
    }
}


void FileStateSimulator::computeFileStateOnePath(Path* path) {
    for (auto bb : *path) {
        computeFileStateOneBasicBlock(bb, path);
    }
}


void FileStateSimulator::handleFunctionCall(CallInst *inst, Path* path) {
    Function* callee = inst->getCalledFunction();

    if (callee->getName().str() == "fopen") {
        for (Use &U : inst->uses()){
            User *useInst = U.getUser();
            if (auto *storeInst = dyn_cast<StoreInst>(useInst)) {
                Value* file = storeInst->getOperand(1);
                string fileStr;
                raw_string_ostream(fileStr) << file;
                pathFileStates[path][fileStr] = "OPEN";
            }
        }
    } else if (callee->getName().str() == "fclose") {
        Value* file = inst->getOperand(0);
        string fileStr;
        raw_string_ostream(fileStr) << file;
        if (pathFileStates[path].find(fileStr) == pathFileStates[path].end()) {
            pathFileStates[path][fileStr] = "ERROR";
        } else {
            pathFileStates[path][fileStr] = "CLOSE";
        }
    }
}


void FileStateSimulator::computeFileStateOneBasicBlock(BasicBlock *bb, Path* path) {
    for (auto inst = bb->getInstList().begin(); inst != bb->getInstList().end(); inst++) {
        if (auto* functionCallInst = dyn_cast<CallInst>(inst)) {
            handleFunctionCall(functionCallInst, path);
        }
    }
}

void FileStateSimulator::printFileStateSimulatorResult() {
    for (Path& p : allPathArray) {
        for (BasicBlock* bb : p) {
            errs() << bb->getName() << "  ";
        }
        errs() << "\n";

        errs() << "The status of files" << "\n";
        for (const auto & state : pathFileStates[&p]) {
            errs() << state.first << " " << state.second << "\n";
        }
        errs() << "\n";
    }

    errs() << "test pass" << "\n";
}


//----------------------------------------------------------
// Register the pass
//---------------------------------------------------------—
static RegisterPass<FileStateSimulator> X("file-state-simulator", "FileStateSimulator Pass",
                                           true, // This pass doesn't modify the CFG => true
                                           false // This pass is not a pure analysis pass => false
);

static llvm::RegisterStandardPasses
        registerFileStateSimulatorPass(PassManagerBuilder::EP_EarlyAsPossible,
                                    [](const PassManagerBuilder &Builder,
                                       legacy::PassManagerBase &PM) {
                                        PM.add(new FileStateSimulator());
                                    });

