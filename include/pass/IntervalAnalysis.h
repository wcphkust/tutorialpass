//
// Created by chasen on 4/6/20.
//

#ifndef HELLO_TRANSFORMATION_INTERVALANALYSIS_H
#define HELLO_TRANSFORMATION_INTERVALANALYSIS_H

#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/SourceMgr.h>
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CallSite.h"
#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/ValueMap.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <list>
#include <set>
#include <unordered_map>
#include <stdint.h>
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/BasicBlock.h"

using namespace llvm;
using namespace std;
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }

namespace IntervalNameSpace{

    struct Interval {
        int lo;
        int hi;

        Interval(int l, int h) : lo(l), hi(h) {}

        Interval() {
            lo = 0;
            hi = -1; // default init to bottom
        }

        bool operator==(const Interval &rhs) const;

        Interval joinInt(Interval a, Interval b);
        Interval wideningInt(Interval oldV, Interval newV);
        Interval addInt(Interval a, Interval b);
        Interval subInt(Interval a, Interval b);

        Interval refineLt(Interval a, Interval b);
        Interval refineLe(Interval a, Interval b);
        Interval refineGe(Interval a, Interval b);
        Interval refineGt(Interval a, Interval b);

        string toStr() const;
    };

    Interval botInt = {0, -1}; // lo >= hi ==> empty set
    Interval topInt = {INT32_MIN, INT32_MAX}; // INT32_MIN means minus infinity, INT32_MAX means plus infinity

    typedef std::unordered_map<Value *, Interval> NumState;

    // At a certain program point (after a instruction),
    // what is the range for llvm values?
    typedef std::unordered_map<Instruction *, NumState> AbstractState;


    //IntAnalysis Pass
    class IntAnalysis: public ModulePass {
        std::set<Value *> caredValues; // all variable appeared in absState
        std::set<Value *> formalArgs;  // all formal arguments in absState
        std::map<Value*, NumState> branchState;
        AbstractState absState;

    public:
        static char ID;

        IntAnalysis() : ModulePass(ID) {};

        virtual bool runOnModule(Module &M);
        void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

        void initArgToTop(Function &F);
        void collectCaredValues();

        void AbstractTransfer(Instruction *inst);
        Interval getJoinIntervalFromPredInst(Instruction* inst, Value* src);
        void mergeIntervalFromPredInst(Instruction* inst, Value* dest);

        void handleOtherInsts(Instruction* inst);
        void handleAllocaInst(AllocaInst* allocaInst);
        void handleLoadInst(LoadInst* loadInst);
        void handleStoreInst(StoreInst* storeInst);
        void handleBranchInst(BranchInst* branchInst);
        void handleBinaryOperator(BinaryOperator *binaryInst);


        void dumpAbstractState(Function &F);
        void printSingleState(Instruction* inst);
        void compute(Function &F);

        // Helper function
        std::vector<Instruction*> findPrecedingProgramPoints(Instruction *inst);

    };
}

#endif //HELLO_TRANSFORMATION_INTERVALANALYSIS_H
