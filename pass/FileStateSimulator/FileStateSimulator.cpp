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
#include "pass/FileStateSimulator.h"

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
    collectAllPath();
    initializeAllPathState();

    for (auto & p : allPathArray) {
        computeFileStateOnePath(p);
    }

    printFileStateSimulatorResult(F.getName());
}


void FileStateSimulator::collectAllPath() {

}


void FileStateSimulator::initializeAllPathState() {

}


void FileStateSimulator::computeFileStateOnePath(Path &p) {
    for (auto bb : p) {
        computeFileStateOneBasicBlock(bb, pathFileStates[&p]);
    }
}


void FileStateSimulator::computeFileStateOneBasicBlock(BasicBlock *bb, FileStates state) {

}


/*
 *
 */
void FileStateSimulator::printFileStateSimulatorResult(llvm::StringRef FuncName) {
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

