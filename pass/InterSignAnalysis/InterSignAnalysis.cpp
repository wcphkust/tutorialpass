//
// Created by yzhanghw on 2020/5/10.
//

#include "pass/InterSignAnalysis.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include <queue>

/***
 * Register the pass. Modify this part only when necessary
 */

char InterSignAnalysis::ID = 0;
static RegisterPass<InterSignAnalysis> X("intersign", "Interprocedural sign analysis",
                                         true,
                                         true);
static llvm::RegisterStandardPasses Y(
        llvm::PassManagerBuilder::EP_EarlyAsPossible,
        [](const llvm::PassManagerBuilder &Builder,
           llvm::legacy::PassManagerBase &PM) { PM.add(new InterSignAnalysis()); });


void InterSignAnalysis::extractVars(Function *func) {
    for (auto &bb : *func) {
        for (auto &inst: bb) {
            // variable declaration
            if (isa<AllocaInst>(inst)) {
                auto allocaInst = &cast<AllocaInst>(inst);
                // save the variable with an ID
                varSetMap[func].insert(allocaInst->getName().str());
                allVarSetMap[func].insert(allocaInst->getName().str());
            } else if (isa<LoadInst>(inst)) {
                auto loadInst = &cast<LoadInst>(inst);
                allVarSetMap[func].insert(loadInst->getName().str());
                allVarSetMap[func].insert(loadInst->op_begin()->get()->getName().str());
            } else if (isa<StoreInst>(inst)) {
                auto storeInst = &cast<StoreInst>(inst);
                auto storeInst = &cast<StoreInst>(inst);
                auto op = storeInst->op_begin();
                allVarSetMap[func].insert(op->get()->getName().str());
                op++;
                allVarSetMap[func].insert(op->get()->getName().str());
            } else if (isa<BinaryOperator>(inst)) {
                auto binaryInst = &cast<BinaryOperator>(inst);
                auto op = binaryInst->op_begin();
                allVarSetMap[func].insert(binaryInst->getName().str());
                allVarSetMap[func].insert(op->get()->getName().str());
                op++;
                allVarSetMap[func].insert(op->get()->getName().str());
            }
        }

        //add function formal parameter
        for (auto arg_it = func->arg_begin(); arg_it != func->arg_end(); arg_it++) {
            varSetMap[func].insert(arg_it->getName().str());
            allVarSetMap[func].insert(arg_it->getName().str());
        }
    }
}


void InterSignAnalysis::topologicalSort(Function *F, std::vector<BasicBlock *> *bbVector) {
    // implement an algorithm to sort the basicblocks
    // parent should always present before the child
    // Assumption: the CFG is DAG

    BasicBlock *entry = &F->getEntryBlock();
    std::queue<BasicBlock *> bb_queue;
    bb_queue.push(entry);

    while (!bb_queue.empty()) {
        BasicBlock *head = bb_queue.front();
        bbVector->push_back(head);
        for (auto it = succ_begin(head); it != succ_end(head); it++) {
            BasicBlock *bb = *it;
            bb_queue.push(bb);
        }
        bb_queue.pop();
    }
}

string InterSignAnalysis::joinState(string state1, string state2) {
    string state = "top";
    if (state1 == "bot") {
        state = state2;
    } else if (state2 == "bot") {
        state = state1;
    } else if (state1 == state2) {
        state = state1;
    }
    return state;
}

map<string, string> InterSignAnalysis::joinMapState(map<string, string> state1, map<string, string> state2) {
    if (state1.size() == 0)  return state2;
    if (state2.size() == 0)  return state1;

    map<string, string> state;
    for (auto &statePair : state1) {
        state[statePair.first] = joinState(statePair.second, state2[statePair.first]);
    }
    for (auto &statePair : state2) {
        state[statePair.first] = joinState(statePair.second, state1[statePair.first]);
    }
    return state;
}

string InterSignAnalysis::getFunctionTerminatorState(Function *func) {
    //string state = "testState";
    string state = "bot";
    for (auto &bb : *func) {
        //Instruction *inst = bb.getTerminator();
        for (auto &instruction : bb) {
            Instruction* inst = &instruction;
            if (auto* returnInst = dyn_cast<ReturnInst>(inst)) {
                state = joinState(state, signResult[func][&bb][returnInst->getOperand(0)->getName().str()]);
            }
        }
    }
    return state;
}

void InterSignAnalysis::handleCallInst(CallInst *callInst, map<BasicBlock*, map<string, string>>& funcState) {
    string dest = callInst->getName().str();
    BasicBlock* bb = callInst->getParent();
    Function *calleeFunc = callInst->getCalledFunction();

    // functions cannot be null
    if (calleeFunc == nullptr) return;
    if (calleeFunc->isIntrinsic() || calleeFunc->empty()) {
        funcState[bb][dest] = "top";
        return;
    }

    vector<string> actualParaList;

    for (int i = 0; i < callInst->getNumArgOperands(); i++) {
        auto* argOperand = callInst->getArgOperand(i);
        if (auto *constValue = dyn_cast<ConstantInt>(argOperand)) {
            actualParaList.push_back(getConstantIntSign(constValue));
        } else {
            actualParaList.push_back(funcState[callInst->getParent()][argOperand->getName()]);
            //actualParaList.push_back("top");
        }
    }

    if (isSensitive) {
        entryState[calleeFunc] = actualParaList;
        analyzeFunction(calleeFunc);
        funcState[bb][dest] = getFunctionTerminatorState(calleeFunc);
    } else {
        //update the entry state of callee
        if (entryState.find(calleeFunc) == entryState.end()) {
            entryState[calleeFunc] = actualParaList;
        } else {
            for (int i = 0; i < entryState[calleeFunc].size(); i++) {
                entryState[calleeFunc][i] = joinState(entryState[calleeFunc][i], actualParaList[i]);
            }
        }

        //add the callee to the worklist or retrieve the cache
        if (!functionAnalyzedFlag[calleeFunc]) {
            funcState[bb][dest] = "top";
            if (!calleeFunc->isIntrinsic() && !calleeFunc->empty()) {
                funcQueue.push(calleeFunc);
            }
        } else {
            funcState[bb][dest] = getFunctionTerminatorState(calleeFunc);
        }
    }
}

void InterSignAnalysis::handleLoadInst(LoadInst* loadInst, map<BasicBlock*, map<string, string>>& funcState) {
    string dest = loadInst->getName().str();
    BasicBlock* bb = loadInst->getParent();
    Value *operand = loadInst->getOperand(0);
    funcState[bb][dest] = funcState[bb][operand->getName().str()];
}

void InterSignAnalysis::handleStoreInst(StoreInst* storeInst, map<BasicBlock*, map<string, string>>& funcState) {
    Value *operand1 = storeInst->getOperand(0);
    Value *operand2 = storeInst->getOperand(1);
    BasicBlock* bb = storeInst->getParent();

    if (isa<Constant>(operand1)) {
        if (auto *constValue = dyn_cast<ConstantInt>(operand1)) {
            funcState[bb][operand2->getName().str()] = getConstantIntSign(constValue);
        } else {
            //memo: non-integer constant
            funcState[bb][operand2->getName().str()] = "top";
        }
    } else {
        funcState[bb][operand2->getName().str()] = funcState[bb][operand1->getName().str()];
    }
}

void InterSignAnalysis::handleBranchInst(BranchInst* branchInst, map<BasicBlock*, map<string, string>>& funcState) {
    //Honor code from ZHOUAN
    if (!branchInst->isConditional()) {
        return;
    }
    Function* f = branchInst->getParent()->getParent();
    if (auto* icmpInst = dyn_cast<ICmpInst>(branchInst->getCondition())) {
        //ICMP_SGT
        if (icmpInst->getSignedPredicate() != CmpInst::ICMP_SGT) {
            return;
        }
        if (auto* loadInst = dyn_cast<LoadInst>(icmpInst->getOperand(0))) {
            string pathVarName = loadInst->getPointerOperand()->getName().str();
            branchVarSetMap[f].insert(pathVarName);
//            funcState[branchInst->getSuccessor(0)][pathVarName] = "+";
//            funcState[branchInst->getSuccessor(1)][pathVarName] = "top";
            branchCondMap[icmpInst->getParent()->getParent()][branchInst->getSuccessor(0)][pathVarName] = "+";
            branchCondMap[icmpInst->getParent()->getParent()][branchInst->getSuccessor(1)][pathVarName] = "top";
        }
    }
}

string InterSignAnalysis::addTwoSigns(string op1, string op2) {
    if (op1 == "bot" || op1 == "top" || op2 == "bot" || op2 == "top")  return "top";

    if (op1 == "0")  return op2;
    if (op2 == "0")  return op1;

    if (op1 == "+" && op2 == "+")  return "+";
    if (op1 == "-" && op2 == "-")  return "-";
    if (op1 == "+" && op2 == "-")  return "top";
    if (op1 == "-" && op2 == "+")  return "top";
}

string InterSignAnalysis::subTwoSigns(string op1, string op2) {
    if (op1 == "bot" || op1 == "top" || op2 == "bot" || op2 == "top")  return "top";

    if (op1 == "0" && op2 == "0")  return "0";
    if (op1 == "0")  return (op2 == "+" ? "-" : "+");
    if (op2 == "0")  return (op2 == "-" ? "+" : "-");

    if (op1 == "+" && op2 == "+")  return "top";
    if (op1 == "-" && op2 == "-")  return "top";
    if (op1 == "+" && op2 == "-")  return "+";
    if (op1 == "-" && op2 == "+")  return "-";
}

void InterSignAnalysis::handleBinaryOperator(BinaryOperator *binaryInst, map<BasicBlock*, map<string, string>>& funcState) {
    BasicBlock* bb = binaryInst->getParent();
    string sign1, sign2;
    //arithmetic instruction
    if (binaryInst->getOpcode() == Instruction::Add || binaryInst->getOpcode() == Instruction::Sub) {
        Value *operand1 = binaryInst->getOperand(0);
        Value *operand2 = binaryInst->getOperand(1);

        //Get the signs of operands
        if (auto *consop1 = dyn_cast<ConstantInt>(operand1)) {
            sign1 = getConstantIntSign(consop1);
        } else {
            sign1 = funcState[bb][operand1->getName().str()];
        }

        if (auto *consop2 = dyn_cast<ConstantInt>(operand2)) {
            sign2 = getConstantIntSign(consop2);
        } else {
            sign2 = funcState[bb][operand2->getName().str()];
        }
    }

    //Get the sign of the result
    string dest = binaryInst->getName().str();
    if (binaryInst->getOpcode() == Instruction::Add) {
        string str = addTwoSigns(sign1, sign2);
        funcState[bb][dest] = str;
        string str2 = funcState[bb][dest];
    } else if (binaryInst->getOpcode() == Instruction::Sub) {
        funcState[bb][dest] = subTwoSigns(sign1, sign2);
    }
}


void InterSignAnalysis::intraBBAnalyze(BasicBlock *bb, map<BasicBlock*, map<string, string>>& funcState) {
    Function* f = bb->getParent();
    //initilize the state
    if (bb == &f->getEntryBlock()) {
        //bb is the entry node
        for (auto var : allVarSetMap[f]) {
            funcState[bb][var] = "bot";
        }
        //initiliazed the parameter
        for (int i = 0; i < entryState[f].size(); i++) {
            string para = f->getArg(i)->getName().str();
            funcState[bb][para] = entryState[f][i];
        }
    } else {
        //bb is not the entry node
        map<string, string> joinState;
        for (auto it = pred_begin(bb); it != pred_end(bb); it++) {
            joinState = joinMapState(joinState, funcState[*it]);
        }
        funcState[bb] = joinState;

        //update the branch condition var via branchCondMap
        map<string, string> condMap = branchCondMap[bb->getParent()][bb];
        for (auto & it : condMap) {
            errs() << bb->getName().str() << "\n";
            errs() << it.second << "\n";
            funcState[bb][it.first] = it.second;
        }
    }

    for (auto &instruction : *bb) {
        if (auto* callInst = dyn_cast<CallInst>(&instruction)) {
            handleCallInst(callInst, funcState);
        } else if (auto* storeInst = dyn_cast<StoreInst>(&instruction)) {
            handleStoreInst(storeInst, funcState);
        } else if (auto* loadInst = dyn_cast<LoadInst>(&instruction)) {
            handleLoadInst(loadInst, funcState);
        } else if (auto* branchInst = dyn_cast<BranchInst>(&instruction)) {
            handleBranchInst(branchInst, funcState);
        } else if (auto* binaryInst = dyn_cast<BinaryOperator>(&instruction)) {
            handleBinaryOperator(binaryInst, funcState);
        }
    }
}

string InterSignAnalysis::getConstantIntSign(ConstantInt *var) {
    if (var->isNegative()) return "-";
    if (var->isZero()) return "0";
    return "+";
}

/***
 * Analyze a function for signs
 * @param func
 */
// you will need to modify the signature of this function
void InterSignAnalysis::analyzeFunction(Function *func) {
    // extract the variables
    // we save the `id -> varname`
    // you could assume we only have declarations in `main`
    bool fixed = true;

    // sort the basicblocks in `main`
    auto mainBBVector = new std::vector<BasicBlock *>();

    //the state of function: map basicblock to sign mapping
    map<BasicBlock*, map<string, string>> fState;

    //assume that there is no loop in each function
    topologicalSort(func, mainBBVector);

    // analyze each basicblock in `main`
    for (auto &bb : *mainBBVector) {
        // your code goes here
        intraBBAnalyze(bb, fState);
        if (!isFunctionFixedPoint(signResult[func][bb], fState[bb])) {
            fixed = false;
        }
        signResult[func][bb] = fState[bb];
    }

    functionAnalyzedFlag[func] = true;
    functionFixedPointFlag[func] = fixed;
}

bool InterSignAnalysis::isFunctionFixedPoint(map<string, string> &s1, map<string, string> &s2) {
    for (auto &p: s2) {
        if (s1.find(p.first) == s1.end()) {
            return false;
        } else if (s1[p.first] != p.second) {
            return false;
        }
    }
    return true;
}

void InterSignAnalysis::generateCallerList(Module &M) {
    auto cg = &getAnalysis<CallGraphWrapperPass>().getCallGraph();

    for (auto &cgn : *cg) {
        Function *func = const_cast<Function *>(cgn.first);
        if (func == nullptr) continue;
        // remove llvm-inserted functions
        if (func->isIntrinsic()) continue;
        //remove lib function
        if (func->empty()) continue;

        allFuncSet.insert(func);
        CallGraphNode *funcNode = cg->operator[](func);

        for (int i = 0; i < funcNode->size(); i++) {
            Function *calledFunc = funcNode->operator[](i)->getFunction();
            if (calledFunc == nullptr) continue;
            if (calledFunc->isIntrinsic()) continue;
            if (calledFunc->empty()) continue;
            allFuncSet.insert(calledFunc);
            callerInfo[calledFunc].insert(func);
        }
    }
    generateTopFunctionList();
}

void InterSignAnalysis::generateTopFunctionList() {
    for (auto &it : allFuncSet) {
        if (callerInfo.find(it) == callerInfo.end()) {
            entryFunction.push_back(it);
        }
    }
}

bool InterSignAnalysis::runOnModule(Module &M) {
    isSensitive = false;
    generateCallerList(M);

    //extract variables in each function
    for (auto &func : entryFunction) {
        funcQueue.push(func);
    }

    for (auto &Func : M) {
        Function *func = &Func;
        if (func == nullptr) continue;
        // remove llvm-inserted functions
        if (func->isIntrinsic()) continue;
        //remove lib function
        if (func->empty()) continue;

        functionAnalyzedFlag[func] = false;
        functionFixedPointFlag[func] = false;
    }

    //initialize the variable set
    for (auto &func : allFuncSet) {
        extractVars(func);
    }

    //Get the call graph and topological sort the functions
    while (!funcQueue.empty()) {
        Function *head = funcQueue.front();
        analyzeFunction(head);
        if (!functionFixedPointFlag[head]) {
            //Add the caller of head to functions
            for (auto caller : callerInfo[head]) {
                funcQueue.push(caller);
            }
        }
        funcQueue.pop();
    }

    reportResult(M);
    return true;
}

bool InterSignAnalysis::doFinalization(Module &M) {
    // report the analysis result before exit
    return true;
}


void InterSignAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    // need the analysis results
    // your code goes here
    AU.addRequired<CallGraphWrapperPass>();
    // not changing the results
    AU.setPreservesAll();
}


void InterSignAnalysis::reportResult(Module &M) {
    // your code goes here

    for (auto func : entryFunction) {
        set<string> varSet = varSetMap[func];
        errs() << "--------------------------------------------" << "\n";
        errs() << "Function name: " << func->getName().str() << "\n";
        errs() << "--------------------------------------------" << "\n";
        map<BasicBlock *, map<string, string>> function_state = signResult[func];
        for (auto subit = function_state.begin(); subit != function_state.end(); subit++) {
            BasicBlock *bb = subit->first;
            map<string, string> bbstate = subit->second;
            errs() << bb->getName().str() << ":" << "\n";
            for (auto subsubit = bbstate.begin(); subsubit != bbstate.end(); subsubit++) {
                string varName = subsubit->first;
                if (varSet.find(varName) != varSet.end()) {
                    errs() << varName << " " << subsubit->second << "\n";
                }
            }
            errs() << "\n";
        }
        errs() << "\n";
    }
}

//debug helper
void InterSignAnalysis::printStrStrMap(map<string, string> state) {
    for (auto it = state.begin(); it != state.end(); it++) {
        errs() << it->first << " " << it->second << "\n";
    }
}
