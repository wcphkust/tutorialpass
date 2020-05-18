//
// Created by yzhanghw on 2020/5/10.
//

#ifndef LLVM_PASS_INTERSIGNANALYSIS_H
#define LLVM_PASS_INTERSIGNANALYSIS_H

#include <string>
#include <map>
#include <queue>
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;
using namespace std;

class InterSignAnalysis : public ModulePass {
public:
    static char ID;
    bool isSensitive; //true if path&context sensitive, otherwise false
    map<Function*, set<string>> varSetMap;
    map<Function*, set<string>> branchVarSetMap;  //store the branch variable
    map<Function*, set<string>> allVarSetMap; //include tmp variables

    vector<Function*> entryFunction;
    set<Function*> allFuncSet;
    map<Function*, set<Function*>> callerInfo;

    queue<Function*> funcQueue;
    map<Function*, bool> functionFixedPointFlag;  //whether the function reached fixed point
    map<Function*, bool> functionAnalyzedFlag;  //whether the function is analyzed

    map<Function*, vector<string>> entryState; //the sign state of function parameters
    map<Function*, map<BasicBlock*, map<string, string>>> branchCondMap;    //the branch condition of basic block
    map<Function*, map<BasicBlock*, map<string, string>>> signResult;


public:
    InterSignAnalysis():ModulePass(ID){}
    bool runOnModule(Module& M) override;
    bool doFinalization(Module& M) override;
    void getAnalysisUsage(AnalysisUsage& AU) const override; // your code goes here

    void topologicalSort(Function* F, std::vector<BasicBlock*>* bbVector);
    void generateCallerList(Module& M);
    void generateTopFunctionList();
    void extractVars(Function* func);

    //lattice
    string getConstantIntSign(ConstantInt* var);
    string addTwoSigns(string op1, string op2);
    string subTwoSigns(string op1, string op2);

    void analyzeFunction(Function* func); // your code goes here
    void intraBBAnalyze(BasicBlock* bb, map<BasicBlock*, map<string, string>>& funcState); // your code goes here
    string getFunctionTerminatorState(Function* func);
    string joinState(string state1, string state2);
    map<string, string> joinMapState(map<string, string> state1, map<string, string> state2);
    bool isFunctionFixedPoint(map<string, string>& s1, map<string, string>& s2);

    //Process each instruction
    void handleCallInst(CallInst* callInst, map<BasicBlock*, map<string, string>>& funcState);
    void handleLoadInst(LoadInst* loadInst, map<BasicBlock*, map<string, string>>& funcState);
    void handleStoreInst(StoreInst* storeInst, map<BasicBlock*, map<string, string>>& funcState);
    void handleBranchInst(BranchInst* branchInst, map<BasicBlock*, map<string, string>>& funcState);
    void handleBinaryOperator(BinaryOperator* binaryInst, map<BasicBlock*, map<string, string>>& funcState);

    void reportResult(Module &M); // your code goes here

    //for debug use
    void printStrStrMap(map<string, string> state);
};


#endif //LLVM_PASS_INTERSIGNANALYSIS_H

//Remark: Flow sensitive: consider path condition(flag), flag > 0 in one block and flag <= 0 in the other
