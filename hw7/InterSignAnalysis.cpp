//
// Created by yzhanghw on 2020/5/10.
//

#include "InterSignAnalysis.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instruction.def"
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
    for (auto& bb : *func) {
        for (auto& inst: bb) {
            // variable declaration
            if (isa<AllocaInst>(inst)) {
                auto allocaInst = &cast<AllocaInst>(inst);
                // save the variable with an ID
                varMap[allocaInst->getName()] = InterSignAnalysis::varCount++;
            }
        }
    }
}


void InterSignAnalysis::extractAllVars(Function *func) {
    for (auto& bb : *func) {
        for (auto& inst : bb) {
            if (isa<AllocaInst>(inst)) {
                // save the variable with an ID
                auto allocaInst = &cast<AllocaInst>(inst);
                allVarMap[allocaInst->getName()] = allVarCount++;
            } else if (isa<LoadInst>(inst)) {
                auto loadInst = &cast<LoadInst>(inst);
                allVarMap[loadInst->getName()] = allVarCount++;
                allVarMap[loadInst->op_begin()->get()->getName()] = allVarCount++;
            } else if (isa<StoreInst>(inst)) {
                auto storeInst = &cast<StoreInst>(inst);
                auto op = storeInst->op_begin();
                allVarMap[op->get()->getName()] = allVarCount++;
                op++;
                allVarMap[op->get()->getName()] = allVarCount++;
            } else if (isa<BinaryOperator>(inst)) {
                BinaryOperator* binaryInst = &cast<BinaryOperator>(inst);
                allVarMap[binaryInst->getName()] = allVarCount++;

                //Operands of binary operators
                auto op = binaryInst->op_begin();
                allVarMap[op->get()->getName()] = allVarCount++;
                op++;
                allVarMap[op->get()->getName()] = allVarCount++;
            }
        }
    }
}


void InterSignAnalysis::topologicalSort(Function* F, std::vector<BasicBlock*>* bbVector) {
    // implement an algorithm to sort the basicblocks
    // parent should always present before the child
    // Assumption: the CFG is DAG

    BasicBlock* entry = &F->getEntryBlock();
    std::queue<BasicBlock*> bb_queue;
    bb_queue.push(entry);

    while(!bb_queue.empty()) {
        BasicBlock* head = bb_queue.front();
        bbVector->push_back(head);
        for (auto it = succ_begin(head); it != succ_end(head); it++) {
            BasicBlock* bb = *it;
            bb_queue.push(bb);
        }
        bb_queue.pop();
    }
}

string InterSignAnalysis::searchForResultBackward(BasicBlock* bb, unsigned operandIndexInAllVarMap) {
    Function* func = bb->getParent();
    queue<BasicBlock*> transitiveBBqueue;
    string sign = "null";

    for (auto it = pred_begin(bb); it != pred_end(bb); it++) {
        transitiveBBqueue.push(*it);
    }

    while(not transitiveBBqueue.empty()) {
        BasicBlock* head = transitiveBBqueue.front();
        map<unsigned, std::string> res = signResult[func][head];
        if (res.find(operandIndexInAllVarMap) != res.end()) {
            sign = res[operandIndexInAllVarMap];
            break;
        } else {
            for (auto it = pred_begin(head); it != pred_end(head); it++) {
                transitiveBBqueue.push(*it);
            }
        }
    }

    return sign;
}

DenseMap<BasicBlock*, DenseMap<StringRef, StringRef>>* InterSignAnalysis::intraBBAnalyze(BasicBlock* bb){
    // outer map: basicblock -> states of vars
    // innter map: varname -> state string (e.g "0" in {"0", "+", "0", "⊤", "⊥"})
    map<unsigned, string> bbSignResult;
    for (auto& instruction : *bb) {
        if (isa<CallInst>(instruction)) {
            auto callInst = &cast<CallInst>(instruction);
            errs() << "function call" << "\n";
            instruction.print(errs());
            errs() << "\n\n";
            // your code goes here
            // analyze this function
        } else if (isa<StoreInst>(instruction)) {
            errs() << "def a variable" << "\n";
            instruction.print(errs());
            errs() << "\n";
            auto* storeInst = &cast<StoreInst>(instruction);
            Value *operand1 = storeInst->getOperand(0);
            Value *operand2 = storeInst->getOperand(1);

            if (auto *constValue = dyn_cast<Constant>(operand1)) {
                if (not constValue->isNegativeZeroValue()) {
                    bbSignResult[allVarMap[operand2->getName()]] = "+";
                } else if (constValue->isZeroValue()) {
                    bbSignResult[allVarMap[operand2->getName()]] = "0";
                } else {
                    bbSignResult[allVarMap[operand2->getName()]] = "-";
                }
            } else {
                unsigned index1 = allVarMap[operand1->getName()];
                if (bbSignResult.find(index1) != bbSignResult.end()) {
                    bbSignResult[allVarMap[operand2->getName()]] = bbSignResult[index1];
                } else {
                    bbSignResult[allVarMap[operand2->getName()]] = searchForResultBackward(bb, index1);
                }
            }
            errs() << operand2->getName().str() << " " << bbSignResult[allVarMap[operand2->getName()]] << "\n\n";
        } else if (isa<LoadInst>(instruction)) {
            errs() << "load a variable" << "\n";
            instruction.print(errs());
            errs() << "\n";
            auto* loadInst = &cast<LoadInst>(instruction);

            StringRef dest = loadInst->getName();
            Value* operand = loadInst->getOperand(0);
            unsigned index = allVarMap[operand->getName()];
            if (bbSignResult.find(index) != bbSignResult.end()) {
                bbSignResult[allVarMap[dest]] = bbSignResult[index];
            } else {
                bbSignResult[allVarMap[dest]] = searchForResultBackward(bb, index);
            }
            errs() << dest.str() << " " << bbSignResult[allVarMap[dest]] << "\n\n";
        } else {
            //arithmetic instruction
            if (instruction.getOpcode() == Instruction::Add || instruction.getOpcode() == Instruction::Sub) {
                Value *operand1 = instruction.getOperand(0);
                Value *operand2 = instruction.getOperand(1);
                string sign1, sign2;

                //Get the signs of operands
                unsigned index1 = allVarMap[operand1->getName()];
                if (bbSignResult.find(index1) != bbSignResult.end()) {
                    sign1 = bbSignResult[index1];
                } else {
                    sign1 = searchForResultBackward(bb, allVarMap[operand1->getName()]);
                }

                unsigned index2 = allVarMap[operand2->getName()];
                if (bbSignResult.find(index2) != bbSignResult.end()) {
                    sign2 = bbSignResult[index2];
                } else {
                    sign2 = searchForResultBackward(bb, index2);
                }

                //Get the sign of the result
                StringRef dest = instruction.getName();
                if (instruction.getOpcode() == Instruction::Add) {
                    errs() << "add two variables" << "\n";
                    if (sign1 == "bot" || sign1 == "top" || sign2 == "bot" || sign2 == "top") {
                        bbSignResult[allVarMap[dest]] = "top";
                    }
                    if (sign1 == "+" && sign2 != "-") {
                        bbSignResult[allVarMap[dest]] = "+";
                    } else if (sign1 != "-" && sign2 == "+") {
                        bbSignResult[allVarMap[dest]] = "-";
                    } else if (sign1 == "-" && sign2 != "+") {
                        bbSignResult[allVarMap[dest]] = "-";
                    } else if (sign1 != "+" && sign2 == "-") {
                        bbSignResult[allVarMap[dest]] = "-";
                    } else if (sign1 == "0" && sign2 == "0") {
                        bbSignResult[allVarMap[dest]] = "0";
                    } else {
                        bbSignResult[allVarMap[dest]] = "top";
                    }
                } else if (instruction.getOpcode() == Instruction::Sub) {
                    errs() << "subtract two variables" << "\n";
                    if (sign1 == "bot" || sign1 == "top" || sign2 == "bot" || sign2 == "top") {
                        bbSignResult[allVarMap[dest]] = "top";
                    }
                    if (sign1 == "+" && sign2 != "+") {
                        bbSignResult[allVarMap[dest]] = "+";
                    } else if (sign1 == "-" && sign2 != "-") {
                        bbSignResult[allVarMap[dest]] = "-";
                    } else if (sign1 == "0" && sign2 == "-") {
                        bbSignResult[allVarMap[dest]] = "+";
                    } else if (sign1 == "0" && sign2 == "+") {
                        bbSignResult[allVarMap[dest]] = "-";
                    } else if (sign1 == "0" && sign2 == "0") {
                        bbSignResult[allVarMap[dest]] = "0";
                    } else {
                        bbSignResult[allVarMap[dest]] = "top";
                    }
                }
                instruction.print(errs());
                errs() << "\n";
                errs() << dest.str() << " " << bbSignResult[allVarMap[dest]] << "\n\n";
            }
        }
    }

    // mock return value
    return new DenseMap<BasicBlock*, DenseMap<StringRef, StringRef>>();
}

/***
 * Analyze a function for signs
 * @param func
 */
// you will need to modify the signature of this function
void InterSignAnalysis::analyzeFunction(Function* func) {
    // extract the variables
    // we save the `id -> varname`
    // you could assume we only have declarations in `main`
    extractVars(func);
    errs() << "extract variables \n";

    extractAllVars(func);
    errs() << "extract all variables \n";

    // sort the basicblocks in `main`
    auto mainBBVector = new std::vector<BasicBlock*>();
    topologicalSort(func, mainBBVector);

    // analyze each basicblock in `main`
    for (auto& bb : *mainBBVector) {
        // your code goes here
        intraBBAnalyze(bb);
    }
}

bool InterSignAnalysis::runOnModule(Module& M){
    auto cg = &getAnalysis<CallGraphWrapperPass>().getCallGraph();

    // filter the LLVM functions
    std::vector<Function*> functions;

    for (auto& cgn : *cg) {
        auto *func = const_cast<Function *>(cgn.first);
        // functions cannot be null
        if (func == nullptr) continue;
        // remove llvm-inserted functions
        if (func->isIntrinsic()) continue;
        functions.push_back(func);

        errs() << "function iterator \n";
    }

    // assume the entrance is `main`
    auto name = StringRef("main");
    auto mainFun = M.getFunction(name);
    errs() << "find function " << mainFun->getName() << "\n";

    analyzeFunction(mainFun);

    // your code goes here
    // what should be saved for the results?

    return true;
}

bool InterSignAnalysis::doFinalization(Module &M) {
    // report the analysis result before exit
    reportResult();
    errs() << "Rerporting result: (I'm not implemented)\n";

    return true;
}


void InterSignAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    // need the analysis results
    // your code goes here
    AU.addRequired<CallGraphWrapperPass>();
    // not changing the results
    AU.setPreservesAll();
}


void InterSignAnalysis::reportResult() {
    // your code goes here
}
