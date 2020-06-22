#include <set>
#include <vector>
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "VirtualCallAnalysis.h"
#include "util/demangle.h"

using namespace std;
using namespace llvm;

char VirtualCallAnalysis::ID = 0;

//
//=============================================================================
//  Helper Functions
//=============================================================================
//

int VirtualCallAnalysis::getVtableIndex(CallInst *callinst) {
    /*
      Assume the virtual call looks exactly like this:

       %vfn = getelementptr inbounds void (%class.B*, i32)*, void (%class.B*, i32)** %vtable, i64 0
       %4 = load void (%class.B*, i32)*, void (%class.B*, i32)** %vfn
       call void %4(%class.B* %2, i32 2)

     */
    if (LoadInst *li = dyn_cast<LoadInst>(callinst->getCalledValue()->stripPointerCasts())) {
        if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(li->getPointerOperand())) {
            if (ConstantInt *ci = dyn_cast<ConstantInt>(GEP->getOperand(1))) {
                return ci->getZExtValue();
            }
        }
    }
    return -1;
}

//
//=============================================================================
//  Analysis Functions
//=============================================================================
//

void VirtualCallAnalysis::buildClassHierarchyInfo(llvm::Module &M) {
    auto structTypes = M.getIdentifiedStructTypes();

    for (auto st : structTypes) {
        classHierarchyMap[st] = set<StructType*>();
        classVirtualTableMap[st] = vector<string>();
    }

    for (auto st : structTypes) {
        for (auto superTy : st->subtypes()) {
            if (StructType *superStTy = dyn_cast<StructType>(superTy)) {
                classHierarchyMap[superStTy].insert(st);
            }
        }
    }
}

void VirtualCallAnalysis::buildVirtualTableInfo(llvm::Module &M) {

    static std::string vtable_for_str = "vtable for";
    static std::string pure_virtual_str = "__cxa_pure_virtual";

    for (auto &gv : M.globals()) {

        // Vtables are constant global variables
        if (!isa<Constant>(gv)) {
            continue;
        }

        string demangled_gv_name = cxx_demangle(gv.getName().str());
        if (demangled_gv_name == "") {
            // the name could not be demangled
            continue;
        }

        // The demangled name of vtables start with "vtable for"
        if (demangled_gv_name.find(vtable_for_str) == std::string::npos) {
            continue;
        }

        // a vtable is a constant array where each array element is a
        if (!gv.hasInitializer()) {
            continue;
        }

        Constant *gv_initializer = gv.getInitializer();

        for (unsigned i = 0, e = gv_initializer->getNumOperands(); i < e; ++i) {

            if (ConstantArray *CA = dyn_cast<ConstantArray>(gv_initializer->getAggregateElement(i))) {
                for (unsigned j = 0; j < CA->getNumOperands(); ++j) {
                    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(CA->getAggregateElement(j))) {
                        if (CE->isCast()) {
                            if (Constant *Cast = ConstantExpr::getBitCast(CE, CE->getType())) {

                                if (Function *VF = dyn_cast<Function>(Cast->getOperand(0))) {

                                    if (VF->getName() != pure_virtual_str) {
                                        // store virtual functions of the class
                                        string classNameStr = demangle_class_name(cxx_demangle(VF->getName().str()));
                                        StructType* structType = M.getTypeByName(llvm_class_name(classNameStr));
                                        classVirtualTableMap[structType].push_back(cxx_demangle(VF->getName().str()));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void VirtualCallAnalysis::classHierarchyAnalysis(std::string varName, int vtableIndex) {
    StructType* stype = varDeclaredMap[varName];
    CHAResult[varName] = set<string>();

    for (auto methodName : classVirtualTableMap[stype]) {
        CHAResult[varName].insert(methodName);
    }

    vector<StructType*> derivedclasses = getDerivedStruct(stype);
    for (auto subclass : derivedclasses) {
        for (auto methodName : classVirtualTableMap[subclass]) {
            CHAResult[varName].insert(methodName);
        }
    }
}

void VirtualCallAnalysis::rapidTypeAnalysis(std::string varName, int vtableIndex) {
    StructType* stype = varDeclaredMap[varName];
    RTAResult[varName] = set<string>();

    if (classAllocated.find(stype) != classAllocated.end()) {
        for (auto methodName : classVirtualTableMap[stype]) {
            RTAResult[varName].insert(methodName);
        }
    }

    vector<StructType*> derivedclasses = getDerivedStruct(stype);
    for (auto subclass : derivedclasses) {
        if (classAllocated.find(subclass) == classAllocated.end()) {
            continue;
        }
        for (auto methodName : classVirtualTableMap[subclass]) {
            RTAResult[varName].insert(methodName);
        }
    }
}

void VirtualCallAnalysis::analyzeCallSite(llvm::Function &F) {
    // analyze every virtual call site
    for (auto & bb : F) {
        for (auto & inst : bb) {
            if (auto callInst = dyn_cast<CallInst>(&inst)) {
                int vTableIndex = getVtableIndex(callInst);
                if (vTableIndex == -1) continue;

                if (auto loadClassInst = dyn_cast<LoadInst>(callInst->getOperand(0))) {
                    string varName = loadClassInst->getOperand(0)->getName().str();
                    classHierarchyAnalysis(varName, vTableIndex);
                    rapidTypeAnalysis(varName, vTableIndex);
                }
            }
        }
    }
}


void VirtualCallAnalysis::collectInfo(llvm::Function &F) {
    for (auto & bb : F) {
        for (auto & inst : bb) {
            if (auto allocInst = dyn_cast<AllocaInst>(&inst)) {
                Type* elemType = allocInst->getAllocatedType()->getPointerElementType();
                if (elemType->isStructTy()) {
                    StructType* stType = (StructType*) elemType;
                    varDeclaredMap[allocInst->getName().str()] = stType;
                }
            }
            if (auto callInst = dyn_cast<CallInst>(&inst)) {
                Function* func = callInst->getCalledFunction();
                if (func) {
                    string str = cxx_demangle(func->getName().str());
                    if (str.find("::") != -1) {
                        string className = llvm_class_name(demangle_class_name(str));
                        StructType* allocaStruct = F.getParent()->getTypeByName(className);
                        classAllocated.insert(allocaStruct);
                    }
                }
            }
        }
    }
}


void VirtualCallAnalysis::analyzeMain(llvm::Function &F) {
    collectInfo(F);
    analyzeCallSite(F);
}

bool VirtualCallAnalysis::runOnModule(llvm::Module &M) {

    buildClassHierarchyInfo(M);
    buildVirtualTableInfo(M);

    for (auto &F : M) {

        if (F.getName() == "main") {
            analyzeMain(F);
        }
    }

    dumpAllResults();

    return false;
}


vector<StructType*> VirtualCallAnalysis::getDerivedStruct(StructType* type) {
    vector<StructType*> derivedStructs;
    if (classHierarchyMap[type].size() == 0) {
        return derivedStructs;
    }
    set<StructType*> derivedStructSet = classHierarchyMap[type];
    for (auto derivedStruct : derivedStructSet) {
        derivedStructs.push_back(derivedStruct);
        vector<StructType*> recursiveDerivedStructs = getDerivedStruct(derivedStruct);
        for (auto recDerivedStruct : recursiveDerivedStructs) {
            derivedStructs.push_back(recDerivedStruct);
        }
    }

    return derivedStructs;
}


void VirtualCallAnalysis::dumpAllResults() {
    errs() << "---------CHA Result---------" << "\n";
    for (auto cha_it : CHAResult) {
        errs() << cha_it.first << ":" << "{";
        for (auto vcall : cha_it.second) {
            errs() << vcall << ", ";
        }
        errs() << "}" << "\n";
    }

    errs() << "---------RTA Result---------" << "\n";
    for (auto rta_it : RTAResult) {
        errs() << rta_it.first << ":" << "{";
        for (auto vcall : rta_it.second) {
            errs() << vcall << ", ";
        }
        errs() << "}" << "\n";
    }
}

static RegisterPass<VirtualCallAnalysis> X("virtual-call", "VirtualCallAnalysis Pass",
                                           true, // This pass doesn't modify the CFG => true
                                           false // This pass is not a pure analysis pass => false
);

static llvm::RegisterStandardPasses
        registerBBinLoopCounterPass(PassManagerBuilder::EP_EarlyAsPossible,
                                    [](const PassManagerBuilder &Builder,
                                       legacy::PassManagerBase &PM) {
                                        PM.add(new VirtualCallAnalysis());
                                    });
