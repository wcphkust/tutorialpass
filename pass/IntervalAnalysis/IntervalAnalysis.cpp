//
// Created by chasen on 4/6/20.
//

#include "pass/IntervalAnalysis.h"
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
#include <llvm/IR/InstrTypes.h>
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/BasicBlock.h"

using namespace IntervalNameSpace;

//----------------------------------------------------------------------------//
// The implementation of Interval //
//----------------------------------------------------------------------------//
Interval Interval::joinInt(Interval a, Interval b) {
    if (a == botInt)
        return b;
    if (b == botInt)
        return a;

    int lo = std::min(a.lo, b.lo), hi = std::max(a.hi, b.hi);
    return Interval(lo,hi);
}

Interval Interval::wideningInt(Interval oldV, Interval newV) {
    int lo = (oldV.lo > newV.lo) ? INT32_MIN : oldV.lo;
    int hi = (oldV.hi < newV.hi) ? INT32_MAX : oldV.hi;
    Interval wideningResult (lo, hi);
    return wideningResult;
}


Interval Interval::addInt(Interval a, Interval b) {
    int lo = a.lo + b.lo;
    int hi = a.hi + b.hi;
    Interval addResult (lo, hi);
    return addResult;
}


Interval Interval::subInt(Interval a, Interval b) {
    int lo = a.lo - b.lo;
    int hi = a.hi - b.hi;
    Interval subResult (lo, hi);
    return subResult;
}

// refina a using relation a < b
Interval Interval::refineLt(Interval a, Interval b) {
    if (a == botInt || b == botInt)  return a;

    //when b is a constant
    if (a.lo < b.hi) {
        if (b.hi == INT32_MAX) { // we view it as plus infinity
            return Interval(a.lo, b.hi);
        } else {
            return Interval(a.lo, b.hi-1);
        }
    } else {
        return botInt;
    }
}

// refina a using relation a >= b
Interval Interval::refineGe(Interval a, Interval b) {
    if (a == botInt || b == botInt)  return a;

    //when b is a constant, i.e., b.lo = b.hi
    if (a.lo <= b.hi) {
        if (a.hi >= b.hi) {
            return Interval(b.lo, a.hi);
        } else {
            return botInt;
        }
    }

    return a;
}

// refina a using relation a <= b
Interval Interval::refineLe(Interval a, Interval b) {
    if (a == botInt || b == botInt)  return a;

    //when b is a constant
    if (a.lo <= b.lo) {
        if (a.hi > b.lo) {
            return Interval(a.lo, b.lo);
        } else {
            return a;
        }
    }

    return botInt;
}

// refina a using relation a > b
Interval Interval::refineGt(Interval a, Interval b) {
    if (a == botInt || b == botInt)  return a;

    //when b is a constant
    if (a.lo < b.lo) {
        if (a.hi < b.lo) {  //we view it as negative infinity
            return botInt;
        } else {
            return Interval(b.lo + 1, a.hi);
        }
    } else {
        return a;
    }
}


bool Interval::operator==(const Interval &rhs) const {
    return lo == rhs.lo && hi == rhs.hi;
}


string Interval::toStr() const {
    std::string res = "[";
    if (lo == INT32_MIN)
        res += "-inf";
    else
        res += std::to_string(lo);

    res += ",";
    if (hi == INT32_MAX)
        res += "inf";
    else
        res += std::to_string(hi);

    res += "]";
    return res;
}


//----------------------------------------------------------------------------//
// The implementation of IntAnalysis //
//----------------------------------------------------------------------------//

char IntAnalysis::ID = 0;

void IntAnalysis::initArgToTop(Function &F) {
    for (auto iter=F.arg_begin(); iter != F.arg_end(); ++iter) {
        Value *formalArg = &*iter;

        if (formalArg->getType()->isIntegerTy()) {
            //absState[nullptr][formalArg] = topInt;
            Instruction* inst = &F.getEntryBlock().front();
            absState[inst][formalArg] = topInt;

            errs() << "Size of abstract state: " << absState.size() << "\n";
            formalArgs.insert(formalArg);

            errs() << "Init success" << "\n";
            errs() << formalArg->getName().str() << "\n";
            inst->print(errs());
            errs() << "\n";
        }
    }
}


void IntAnalysis::collectCaredValues() {
    for (auto &inst_m : absState) {
        for (auto &val_itv : inst_m.second) {
            caredValues.insert(val_itv.first);
        }
    }
}


/*
 * This procedure should model absState's transfer function on llvm IR (only those appeared in test.bc)
 * */
void IntAnalysis::AbstractTransfer(Instruction *inst) {
    if (auto* storeInst = dyn_cast<StoreInst>(inst)) {
        handleStoreInst(storeInst);
    } else if (auto* loadInst = dyn_cast<LoadInst>(inst)) {
        handleLoadInst(loadInst);
    } else if (auto* branchInst = dyn_cast<BranchInst>(inst)) {
        handleBranchInst(branchInst);
    } else if (auto* binaryInst = dyn_cast<BinaryOperator>(inst)) {
        handleBinaryOperator(binaryInst);
    } else if (auto* allocaInst = dyn_cast<AllocaInst>(inst)) {
        handleAllocaInst(allocaInst);
    } else {
        handleOtherInsts(inst);
    }
}


Interval IntAnalysis::getJoinIntervalFromPredInst(Instruction* inst, Value* src) {
    vector<Instruction*> preds = findPrecedingProgramPoints(inst);
    Interval srcInterval = botInt;
    if (inst == &inst->getParent()->front()) {
        if (branchState[inst].find(src) != branchState[inst].end()) {
            errs() << "first inst" << "\n";
            srcInterval = branchState[inst][src];
        }
    }

    if (formalArgs.find(src) != formalArgs.end()) {
        return topInt;
    }

    if (auto *srcConst = dyn_cast<ConstantInt>(src)) {
        Interval compactInterval(srcConst->getSExtValue(), srcConst->getSExtValue());
        return compactInterval;
    }

    errs() << "join the interval from pred inst" << "\n";
    errs() << srcInterval.toStr() << "\n";

    for (Instruction* predInst : preds) {
        if (predInst == nullptr) {
            continue;
        }
        if (absState.find(predInst) == absState.end()) {
            continue;
        }

        if (isa<BranchInst>(predInst)) {
            srcInterval = srcInterval.joinInt(srcInterval, branchState[predInst][src]);
        } else {
            Interval predSourceInterval = absState[predInst][src];

            predInst->print(errs());
            errs() << "\n";
            errs() << predSourceInterval.toStr() << "\n";

            srcInterval = srcInterval.joinInt(srcInterval, predSourceInterval);
        }
    }
    errs() << srcInterval.toStr() << "\n";
    return srcInterval;
}

//merge the interval of the values except src
void IntAnalysis::mergeIntervalFromPredInst(Instruction* inst, Value* dest) {
    vector<Instruction*> preds = findPrecedingProgramPoints(inst);
    int flag = 0;

    for (Instruction* predInst : preds) {
        if (absState.find(predInst) == absState.end()) {
            continue;
        }
        for (auto it : absState[predInst]) {
            Value *var = it.first;
            if (var == dest) continue;
            if (absState[inst].find(var) != absState[inst].end()) {
                absState[inst][var] = absState[inst][var].joinInt(absState[inst][var], it.second);
            } else {
                absState[inst][var] = it.second;
            }
        }
    }
}


void IntAnalysis::handleOtherInsts(Instruction* inst) {
    mergeIntervalFromPredInst(inst, inst);
}


void IntAnalysis::handleAllocaInst(AllocaInst* allocaInst) {
    Value *dest = allocaInst;
    mergeIntervalFromPredInst(allocaInst, dest);
    absState[allocaInst][dest] = topInt;
}


void IntAnalysis::handleLoadInst(LoadInst* loadInst) {
    Value *dest = loadInst;
    Value *src = loadInst->getOperand(0);

    mergeIntervalFromPredInst(loadInst, dest);
    Interval srcInterval = getJoinIntervalFromPredInst(loadInst, src);

    loadInst->print(errs());
    errs() << "\n";
    errs() << srcInterval.toStr() << "\n";

    absState[loadInst][dest] = srcInterval;
}


void IntAnalysis::handleStoreInst(StoreInst* storeInst) {
    Value *src = storeInst->getOperand(0);
    Value *dest = storeInst->getOperand(1);

    if (isa<Constant>(src)) {
        if (auto *constValue = dyn_cast<ConstantInt>(src)) {
            Interval compactInterval (constValue->getSExtValue(), constValue->getSExtValue());
            errs() << "print" << "\n";
            mergeIntervalFromPredInst(storeInst, dest);
            absState[storeInst][dest] = compactInterval;
        }
        return;
    }

    mergeIntervalFromPredInst(storeInst, dest);
    Interval srcInterval = getJoinIntervalFromPredInst(storeInst, src);
    errs() << srcInterval.toStr() << "\n";
    errs() << "SROURCE" << src->getName().str() << "\n";
    absState[storeInst][dest] = srcInterval;
}


void IntAnalysis::handleBranchInst(BranchInst* branchInst) {
    if (!branchInst->isConditional()) {
        return;
    }

    if (auto* icmpInst = dyn_cast<ICmpInst>(branchInst->getCondition())) {
        Value* operand1 = icmpInst->getOperand(0);
        Value* operand2 = icmpInst->getOperand(1);

        branchInst->print(errs());
        errs() << "\n";

        //Assume the second operand is ConstantInt
        if (icmpInst->getSignedPredicate() == CmpInst::ICMP_SLT) {
            //ICMP_SLT
            Interval interval1 = getJoinIntervalFromPredInst(branchInst, operand1);
            Interval interval2 = getJoinIntervalFromPredInst(branchInst, operand2);
            Interval ltTResult = interval1.refineLt(interval1, interval2);
            Interval ltFResult = interval1.refineGe(interval1, interval2);

            errs() << "Interval 1: " << interval1.toStr() << "\n";
            errs() << "Interval 2: " << interval2.toStr() << "\n";
            errs() << "less True: " << ltTResult.toStr() << "\n";
            errs() << "less False: " << ltFResult.toStr() << "\n";


            Instruction* head1 = &(branchInst->getSuccessor(0)->front());
            branchState[head1][operand1] = ltTResult;

            errs() << "True branch: ";
            head1->print(errs());
            errs() << "\n";
            mergeIntervalFromPredInst(head1, head1);
            //absState[head1][operand1] = ltTResult;

            Instruction* head2 = &(branchInst->getSuccessor(1)->front());
            branchState[head2][operand1] = ltFResult;

            errs() << "False branch: ";
            head2->print(errs());
            errs() << "\n";
            mergeIntervalFromPredInst(head2, head2);
            //absState[head2][operand1] = ltFResult;

        } else if (icmpInst->getSignedPredicate() == CmpInst::ICMP_SGT) {
            //ICMP_SGT
            Interval interval1 = getJoinIntervalFromPredInst(branchInst, operand1);
            Interval interval2 = getJoinIntervalFromPredInst(branchInst, operand2);
            Interval gtTResult = interval1.refineGt(interval1, interval2);
            Interval gtFResult = interval1.refineLe(interval1, interval2);

            errs() << "Interval 1: " << interval1.toStr() << "\n";
            errs() << "Interval 2: " << interval2.toStr() << "\n";
            errs() << "less True: " << gtTResult.toStr() << "\n";
            errs() << "less False: " << gtFResult.toStr() << "\n";

            Instruction* head1 = &(branchInst->getSuccessor(0)->front());
            branchState[head1][operand1] = gtTResult;

            mergeIntervalFromPredInst(head1, head1);
            //absState[head1][operand1] = geTResult;

            Instruction* head2 = &(branchInst->getSuccessor(1)->front());
            branchState[head2][operand1] = gtFResult;

            mergeIntervalFromPredInst(head2, head2);
            //absState[head2][operand1] = geFResult;
        }

        printSingleState(branchInst);
    }
}


void IntAnalysis::handleBinaryOperator(BinaryOperator *binaryInst) {
    BasicBlock* bb = binaryInst->getParent();
    Value *operand1, *operand2;
    Interval interval1, interval2;

    binaryInst->print(errs());
    errs() << "\n";

    if (binaryInst->getOpcode() == Instruction::Add || binaryInst->getOpcode() == Instruction::Sub) {
        Value *operand1 = binaryInst->getOperand(0);
        Value *operand2 = binaryInst->getOperand(1);

        if (auto *consop1 = dyn_cast<ConstantInt>(operand1)) {
            interval1 = Interval(consop1->getSExtValue(), consop1->getSExtValue());
            errs() << "pre interval 1" << interval1.toStr() << "\n";
        } else {
            interval1 = getJoinIntervalFromPredInst(binaryInst, operand1);
        }

        if (auto *consop2 = dyn_cast<ConstantInt>(operand2)) {
            interval2 = Interval(consop2->getSExtValue(), consop2->getSExtValue());
            errs() << "pre interval 2" << interval2.toStr() << "\n";
        } else {
            interval2 = getJoinIntervalFromPredInst(binaryInst, operand2);
        }
    }

    Value* dest = binaryInst;
    if (binaryInst->getOpcode() == Instruction::Add) {
        Interval addResult = interval1.addInt(interval1, interval2);

        errs() << "Interval 1" << interval1.toStr() << "\n";
        errs() << "Interval 2" << interval2.toStr() << "\n";
        errs() << "add Result" << addResult.toStr() << "\n";

        mergeIntervalFromPredInst(binaryInst, dest);
        absState[binaryInst][dest] = addResult;
    } else if (binaryInst->getOpcode() == Instruction::Sub) {
        Interval subResult = interval1.subInt(interval1, interval2);

        errs() << "Interval 1" << interval1.toStr() << "\n";
        errs() << "Interval 2" << interval2.toStr() << "\n";
        errs() << "sub Result" << subResult.toStr() << "\n";

        mergeIntervalFromPredInst(binaryInst, dest);
        absState[binaryInst][dest] = subResult;
    }
}


void IntAnalysis::dumpAbstractState(Function& F) {
    auto findInst = [] (const std::vector<Instruction *> &vec) {
        Instruction *res = nullptr;
        for (auto i = vec.rbegin(); i != vec.rend(); ++i) {
            Instruction *cur = *i;
            if (isa<StoreInst> (cur)) {
                res = cur;
                break;
            }
        }
        return res;
    };

    collectCaredValues();
    std::map<int, std::vector<Instruction *>> lineMap;

    for (auto iter = inst_begin(F); iter != inst_end(F); ++iter) {
        Instruction *inst = &*iter;
        const llvm::DebugLoc &debugInfo = inst->getDebugLoc();
        if (debugInfo.get()) {
            int line = debugInfo->getLine();
            lineMap[line].push_back(inst);
        }
    }

    for (auto &line_insts : lineMap) {
        int line = line_insts.first;
        auto instVec = line_insts.second;
        auto inst = findInst(instVec);
        if (!inst) {
            continue;
        }

        llvm::errs() << "Line " << line << ":\n";

        for (auto val : caredValues) {
            if (isa<AllocaInst>(val)) {
                llvm::errs() << cast<AllocaInst>(val)->getName().str() << ": ";
                auto itv = absState[inst][val];
                llvm::errs() << itv.toStr() << "; ";
            }
        }
        llvm::errs() << "\n";
    }
}


void IntAnalysis::compute(Function& F) {
    initArgToTop(F);
    bool hasChanged = false;
    do {
        auto curAbs = absState;
        for (auto iter = inst_begin(F); iter != inst_end(F); ++iter) {
            Instruction *inst = &*iter;
            AbstractTransfer(inst);
        }

        hasChanged = !(curAbs == absState);

    } while (hasChanged);

    dumpAbstractState(F);
}

void IntAnalysis::printSingleState(Instruction* inst) {
    for (auto stateIt : absState[inst]) {
//        if (isa<AllocaInst>(stateIt.first)) {
//            llvm::errs() << cast<AllocaInst>(stateIt.first)->getName().str() << ": ";
//            llvm::errs() << stateIt.second.toStr() << "; ";
//        }
        errs() << stateIt.first->getName().str() << ": ";
        errs() << stateIt.second.toStr() << "; ";
    }
    errs() << "\n";
}


//Helper function
std::vector<Instruction *> IntAnalysis::findPrecedingProgramPoints(Instruction *inst) {
    std::vector<Instruction *> preds;

    BasicBlock *bb = inst->getParent();
    auto beginInst = &*bb->begin();

    if (beginInst == inst) {
        if (&(bb->getParent()->getEntryBlock()) == bb) {
            preds.push_back(nullptr);
            return preds;
        }

        for (auto bbIter = pred_begin(bb); bbIter != pred_end(bb); ++bbIter) {
            BasicBlock *prevBB = *bbIter;
            preds.push_back(prevBB->getTerminator());
        }

    } else {
        auto nextIter = bb->begin();
        for (auto iIter = bb->begin(); iIter != bb->end(); ++iIter) {
            nextIter = iIter;
            ++nextIter;

            if (nextIter != bb->end()) {
                if (inst == &*nextIter) {
                    preds.push_back(&*iIter);
                    break;
                }
            }
        }
    }

    assert(!preds.empty());
    return preds;
}


static RegisterPass<IntAnalysis> X("intanalysis", "Interval Analysis Pass", true, false);

void IntAnalysis::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

bool IntAnalysis::runOnModule(Module &M) {
    for(auto& f:M) {
        if (f.isDeclaration()){
            continue;
        }
        compute(f);
    }
    return false;
}





