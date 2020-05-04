//
// Created by chasen on 4/5/20.
//

#ifndef HELLO_TRANSFORMATION_WORKLIST_H
#define HELLO_TRANSFORMATION_WORKLIST_H

#include <utility>
#include <queue>
#include <map>
#include <vector>
#include <set>
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace std;

namespace PassUtilSpace {

    typedef map<string, unsigned> BitVectorBase;
    typedef deque<Instruction*> (*InstDepsFunc)(Instruction*); //deps for instruction, error prone

    enum AnalyzeDirection {
        FORWARD,
        BACKWORD
    };

    enum ApproximationMode {
        MAY,
        MUST
    };

    template<class T> class WorkList {
    public:
        deque<T*> workList;
        map<T*, BitVector> instPreFactMap;    //pre-state of an instruction
        map<T*, BitVector> instPostFactMap;   //post-state of an instruction
        AnalyzeDirection direction;
        ApproximationMode approxMode;

    public:
        WorkList() = default;
        WorkList(Function &F, BitVectorBase pVarIndex, bool isForward = true, bool isMay = true);

        //Operations on worklist
        bool pushInstToWorkList(T* inst);
        void pushDepsToWorkList(T* node, InstDepsFunc deps);
        void popFromWorkList();
        bool isEmpty();
        T* getWorkListHead();

        //Operations on pre/post fact map
        void insertPreBitVector(T* node, BitVector bv);
        void insertPostBitVector(T* node, BitVector bv);

        //Operations on bit vectors
        static BitVector transferFunction(BitVector &bv, BitVector &Kill, BitVector &Gen);  //Kill-Gen transfer function
        BitVector join(BitVector& bv1, BitVector& bv2);
        bool isFixedPoint(BitVector bv, T* node);    //judge fixed-point by pre-state of a node
    };


    struct InstWorkList {
        deque<Instruction*> workList;
        map<Instruction*, BitVector> instPreFactMap;    //pre-state of an instruction
        map<Instruction*, BitVector> instPostFactMap;   //post-state of an instruction
        AnalyzeDirection direction;
        ApproximationMode approxMode;

        InstWorkList() = default;
        InstWorkList(Function &F, BitVectorBase pVarIndex, bool isForward = true, bool isMay = true);

        //Operations on worklist
        bool pushInstToWorkList(Instruction* inst);
        void pushDepsInstToWorkList(Instruction* inst, InstDepsFunc deps);
        void popInstFromWorkList();
        bool isEmpty();
        Instruction* getWorkListHead();

        //Operations on pre/post fact map
        void insertPreBitVector(Instruction* inst, BitVector bv);
        void insertPostBitVector(Instruction* inst, BitVector bv);

        //Operations on bit vectors
        static BitVector transferFunction(BitVector &bv, BitVector &Kill, BitVector &Gen);  //Kill-Gen transfer function
        BitVector join(BitVector& bv1, BitVector& bv2);
        bool isFixedPoint(BitVector bv, Instruction* inst);    //judge fixed-point by pre-state of an instruction
    };


}

#endif //HELLO_TRANSFORMATION_WORKLIST_H