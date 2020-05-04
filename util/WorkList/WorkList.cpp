//
// Created by chasen on 4/5/20.
//

#include <queue>
#include <map>
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "util/WorkList.h"

using namespace PassUtilSpace;


//----------------------------------------------------------
// Implementation of template WorkList
//----------------------------------------------------------

/*
 * InstWorkList constructor
 * Function &F: the function from which worklist of instruction is created
 * BitVectorBase varIndex: the mapping from variables to index
 * isForward: is forward-analysis
 *   true: initialize by adding the first instruction of entry
 *   false: initialize by adding the first instruction of entry
 */
template<class T> WorkList<T>::WorkList(Function &F, BitVectorBase pVarIndex, bool isForward, bool isMay) {
    direction = (isForward ? FORWARD : BACKWORD);
    approxMode = (isMay ? MAY : MUST);

    if (isForward) {
        auto it = F.getBasicBlockList().begin();
        BasicBlock* entryBB = &(*it);
        Instruction* firstInst = &(entryBB->front());
        instPreFactMap[firstInst] = BitVector(pVarIndex.size(), false);
        workList.push_back(firstInst);
    } else {
        auto it = F.getBasicBlockList().rbegin();
        BasicBlock* exitBB = &(*it);
        Instruction* lastInst = &(exitBB->back());
        instPostFactMap[lastInst] = BitVector(pVarIndex.size(), false);
        workList.push_back(lastInst);
    }
}

/*
 * Kill-Gen transfer function
 * Kill, Gen should be calculated before the invocation in the caller
 */
template<class T> BitVector WorkList<T>::transferFunction(BitVector &bv, BitVector &Kill, BitVector &Gen) {
    BitVector resBv(bv);
    resBv &=  Kill.flip();
    resBv |= Gen;
    return resBv;
}

/*
 * Join operator
 */
template<class T> BitVector WorkList<T>::join(BitVector &bv1, BitVector &bv2) {
    BitVector bv = bv1;
    if (approxMode == MAY) {
        bv |= bv2;
    } else {
        //approxMode == MUST
        bv &= bv2;
    }
    return bv;
}


/*
 * Fixed point checker
 * bv: the state of the instruction after transfermation
 * node: node pointer
 */
template<class T> bool WorkList<T>::isFixedPoint(BitVector bv, T* node) {
    map<T*, BitVector>* factMap = (direction == BACKWORD ? &instPreFactMap : &instPostFactMap);
    if (factMap->find(node) == factMap->end()) {
        return false;
    }
    // Judge whether the original state is covered by the current one(bv)
    BitVector oldBv = (*factMap)[node];
    BitVector joinBv = join(oldBv, bv);
    return (joinBv == oldBv);
}

/*
 * Add node to worklist
 * Return false if node has existed, false otherwise
 */
template<class T> bool WorkList<T>::pushInstToWorkList(T* node) {
    for (auto & it : workList) {
        if (node == it) {
            return false;
        }
    }
    workList.push_back(node);
    return true;
}

/*
 * Add the instructions which depend on inst
 * Update the pre/post-state based on the post/pre-state of inst
 */
template<class T> void WorkList<T>::pushDepsToWorkList(T *node, InstDepsFunc deps) {
    deque<T*> depInsts = deps(node);
    for (auto & depInst : depInsts) {
        pushInstToWorkList(depInst);
        if (direction == BACKWORD) {
            BitVector postBv = instPreFactMap[node];
            insertPostBitVector(depInst, postBv);
        } else {
            //FORWARD
            BitVector preBv = instPostFactMap[node];
            insertPreBitVector(depInst, preBv);
        }
    }
}

/*
 * Remove T from worklist
 */
template<class T> void WorkList<T>::popFromWorkList() {
    return workList.pop_front();
}

/*
 * Get the head of worklist
 */
template<class T> T* WorkList<T>::getWorkListHead() {
    return workList.front();
}

/*
 * Insert bit vector into PreFactMap
 */
template<class T> void WorkList<T>::insertPreBitVector(T* inst, BitVector bv) {
    if (instPreFactMap.count(inst) == 0) {
        instPreFactMap[inst] = bv;
    } else {
        instPreFactMap[inst] = join(instPreFactMap[inst], bv);
    }
}

/*
 * Insert bit vector into PostFactMap
 */
template<class T> void WorkList<T>::insertPostBitVector(T* inst, BitVector bv) {
    if (instPostFactMap.count(inst) == 0) {
        instPostFactMap[inst] = bv;
    } else {
        instPostFactMap[inst] = join(instPostFactMap[inst], bv);
    }
}

/*
 * Judge whether the worklist is empty or not
 * Return: 1 for empty, 0 for non-empty
 */
template<class T> bool WorkList<T>::isEmpty() {
    return workList.empty();
}


//----------------------------------------------------------
// Implementation of InstWorkList
//----------------------------------------------------------

/*
 * InstWorkList constructor
 * Function &F: the function from which worklist of instruction is created
 * BitVectorBase varIndex: the mapping from variables to index
 * isForward: is forward-analysis
 *   true: initialize by adding the first instruction of entry
 *   false: initialize by adding the first instruction of entry
 */
InstWorkList::InstWorkList(Function &F, BitVectorBase pVarIndex, bool isForward, bool isMay) {
    direction = (isForward ? FORWARD : BACKWORD);
    approxMode = (isMay ? MAY : MUST);

    if (isForward) {
        auto it = F.getBasicBlockList().begin();
        BasicBlock* entryBB = &(*it);
        Instruction* firstInst = &(entryBB->front());
        instPreFactMap[firstInst] = BitVector(pVarIndex.size(), false);
        workList.push_back(firstInst);
    } else {
        auto it = F.getBasicBlockList().rbegin();
        BasicBlock* exitBB = &(*it);
        Instruction* lastInst = &(exitBB->back());
        instPostFactMap[lastInst] = BitVector(pVarIndex.size(), false);
        workList.push_back(lastInst);
    }
}

/*
 * Kill-Gen transfer function
 * Kill, Gen should be calculated before the invocation in the caller
 */
BitVector InstWorkList::transferFunction(BitVector &bv, BitVector &Kill, BitVector &Gen) {
    BitVector resBv(bv);
    resBv &=  Kill.flip();
    resBv |= Gen;
    return resBv;
}

/*
 * Join operator
 */
BitVector InstWorkList::join(BitVector &bv1, BitVector &bv2) {
    BitVector bv = bv1;
    if (approxMode == MAY) {
        bv |= bv2;
    } else {
        //approxMode == MUST
        bv &= bv2;
    }
    return bv;
}


/*
 * Fixed point checker
 * bv: the state of the instruction after transfermation
 * inst: instruction pointer
 */
bool InstWorkList::isFixedPoint(BitVector bv, Instruction* inst) {
    map<Instruction*, BitVector>* factMap = (direction == BACKWORD ? &instPreFactMap : &instPostFactMap);
    if (factMap->find(inst) == factMap->end()) {
        return false;
    }
    // Judge whether the original state is covered by the current one(bv)
    BitVector oldBv = (*factMap)[inst];
    BitVector joinBv = join(oldBv, bv);
    return (joinBv == oldBv);
}

/*
 * Add instruction to worklist
 * Return false if inst has existed, false otherwise
 */
bool InstWorkList::pushInstToWorkList(Instruction *inst) {
    for (auto & it : workList) {
        if (inst == it) {
            return false;
        }
    }
    workList.push_back(inst);
    return true;
}

/*
 * Add the instructions which depend on inst
 * Update the pre/post-state based on the post/pre-state of inst
 */
void InstWorkList::pushDepsInstToWorkList(Instruction *inst, InstDepsFunc deps) {
    deque<Instruction*> depInsts = deps(inst);
    for (auto & depInst : depInsts) {
        pushInstToWorkList(depInst);
        if (direction == BACKWORD) {
            BitVector postBv = instPreFactMap[inst];
            insertPostBitVector(depInst, postBv);
        } else {
            //FORWARD
            BitVector preBv = instPostFactMap[inst];
            insertPreBitVector(depInst, preBv);
        }
    }
}

/*
 * Remove Instruction from worklist
 */
void InstWorkList::popInstFromWorkList() {
    return workList.pop_front();
}

/*
 * Get the head of worklist
 */
Instruction* InstWorkList::getWorkListHead() {
    return workList.front();
}

/*
 * Insert bit vector into PreFactMap
 */
void InstWorkList::insertPreBitVector(Instruction* inst, BitVector bv) {
    if (instPreFactMap.count(inst) == 0) {
        instPreFactMap[inst] = bv;
    } else {
        instPreFactMap[inst] = join(instPreFactMap[inst], bv);
    }
}

/*
 * Insert bit vector into PostFactMap
 */
void InstWorkList::insertPostBitVector(Instruction* inst, BitVector bv) {
    if (instPostFactMap.count(inst) == 0) {
        instPostFactMap[inst] = bv;
    } else {
        instPostFactMap[inst] = join(instPostFactMap[inst], bv);
    }
}

/*
 * Judge whether the worklist is empty or not
 * Return: 1 for empty, 0 for non-empty
 */
bool InstWorkList::isEmpty() {
    return workList.empty();
}