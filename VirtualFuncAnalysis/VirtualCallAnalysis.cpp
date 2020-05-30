#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <cxxabi.h>
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "VirtualCallAnalysis.h"

using namespace std;
using namespace llvm;

char VirtualCallAnalysis::ID = 0;

//
//=============================================================================
//  Helper Functions
//=============================================================================
//

// demangle c++ name: _ZN1BC1Ev -> B::B()
static std::string cxx_demangle(const std::string &mangled_name) {

    int status = 0;
    char *demangled = abi::__cxa_demangle(mangled_name.c_str(), NULL, NULL, &status);
    std::string result((status == 0 && demangled != NULL) ? demangled
                                                          : mangled_name);
    free(demangled);
    return result;
}

// get class name from function name: B::B() -> B
static std::string demangle_class_name(const std::string &demangled_name) {

    int found = demangled_name.find("::");
    return demangled_name.substr(0, found);
}

// get class name used by LLVM: A -> class.A
static std::string llvm_class_name(const std::string &classN) {

    return "class." + classN;
}

// get only function name : A::foo(int) -> foo(int)
static std::string get_called_function_name(const std::string s) {

    int found = s.find("::");
    return s.substr(found + 2);
}

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
        for (auto subTy : st->subtypes()) {
            if (StructType *subStTy = dyn_cast<StructType>(subTy)) {
                classHierarchyMap[st].insert(subStTy);
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

        //errs() << gv.getName().str() << "\n";

        for (unsigned i = 0, e = gv_initializer->getNumOperands(); i < e; ++i) {

            if (ConstantArray *CA = dyn_cast<ConstantArray>(gv_initializer->getAggregateElement(i))) {
                //errs() << CA->getName().str() << "\n";

                for (unsigned j = 0; j < CA->getNumOperands(); ++j) {
                    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(CA->getAggregateElement(j))) {
                        if (CE->isCast()) {
                            if (Constant *Cast = ConstantExpr::getBitCast(CE, CE->getType())) {

                                if (Function *VF = dyn_cast<Function>(Cast->getOperand(0))) {

                                    if (VF->getName() != pure_virtual_str) {
                                        //errs() << "Hit" << "\n";
                                        //errs() << CE->getName().str() << "\n";
                                        // store virtual functions of the class
                                        string classNameStr = demangle_class_name(cxx_demangle(VF->getName().str()));
                                        errs() << classNameStr << "\n";
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

        errs() << "\n";
    }
}

void VirtualCallAnalysis::classHierarchyAnalysis(std::string varName, int vtableIndex) {
    //TODO
    //std::map<StructType*, std::vector<std::string>> classVirtualTableMap;
    //std::map<StructType*, std::set<StructType*>> classHierarchyMap;
    //errs() << varName << "\n";
    StructType* stype = varDeclaredMap[varName];
    CHAResult[varName] = set<string>();
    errs() << stype->getName().str() << "\n";

    for (auto methodName : classVirtualTableMap[stype]) {
        CHAResult[varName].insert(methodName);
        errs() << methodName << "\n";
    }

    vector<StructType*> derivedclasses = getDerivedStruct(stype);
    for (auto subclass : derivedclasses) {
        for (auto methodName : classVirtualTableMap[subclass]) {
            CHAResult[varName].insert(methodName);
            errs() << methodName << "\n";
        }
    }
}

void VirtualCallAnalysis::rapidTypeAnalysis(std::string varName, int vtableIndex) {


    //TODO
}

void VirtualCallAnalysis::analyzeCallSite(llvm::Function &F) {
    // analyze every virtual call site
    for (auto & bb : F) {
        for (auto & inst : bb) {
            if (auto callInst = dyn_cast<CallInst>(&inst)) {
                int vTableIndex = getVtableIndex(callInst);
                if (vTableIndex == -1) continue;
                errs() << "Call Site" << "\n";
                callInst->print(errs());
                errs() << vTableIndex << "\n";

                if (auto loadClassInst = dyn_cast<LoadInst>(callInst->getOperand(0))) {
                    string varName = loadClassInst->getOperand(0)->getName().str();
                    errs() << varName << "\n";
                    classHierarchyAnalysis(varName, vTableIndex);
                    rapidTypeAnalysis(varName, vTableIndex);
                }
            }
        }
    }
}


void VirtualCallAnalysis::collectInfo(llvm::Function &F) {

    // collect declare and object allocation information
    errs() << "collect Info" << "\n";

    for (auto & bb : F) {
        for (auto & inst : bb) {
            if (auto allocInst = dyn_cast<AllocaInst>(&inst)) {
                Type* elemType = allocInst->getAllocatedType()->getPointerElementType();
                if (elemType->isStructTy()) {
                    StructType* stType = (StructType*) elemType;
                    errs() << "Console varDeclaredMap: " << allocInst->getName().str() << "->" << stType->getName().str() << "\n";
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
                        allocaStruct->print(errs());
                    }
                }
            }
        }
    }

    errs() << "\n";
}


void VirtualCallAnalysis::analyzeMain(llvm::Function &F) {

    collectInfo(F);
    analyzeCallSite(F);
}

bool VirtualCallAnalysis::runOnModule(llvm::Module &M) {

    buildClassHierarchyInfo(M);

    for (auto type : classHierarchyMap) {
        for (auto subType : type.second) {
            errs() << type.first->getName().str() << ":" << subType->getName().str() << "\n";
        }
    }


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

    //TODO
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
