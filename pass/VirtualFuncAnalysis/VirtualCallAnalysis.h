#include <utility>
#include <queue>
#include <map>
#include <vector>
#include <set>
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

class VirtualCallAnalysis : public llvm::ModulePass {
public:
    static char ID;
    VirtualCallAnalysis():ModulePass(ID){}
    bool runOnModule(llvm::Module &M) override;

public:
	int getVtableIndex(CallInst *callinst);
	void classHierarchyAnalysis(std::string varName, int vtableIndex);
	void rapidTypeAnalysis(std::string varName, int vtableIndex);

public:
	void buildClassHierarchyInfo(llvm::Module &M);
	void buildVirtualTableInfo(llvm::Module &M);
	void analyzeMain(llvm::Function &F);
	void collectInfo(llvm::Function &F);
	void analyzeCallSite(llvm::Function &F);
	std::vector<StructType*> getDerivedStruct(StructType* type);
	void dumpAllResults();

private:
	std::map<std::string, StructType*> varDeclaredMap;
	std::map<StructType*, std::set<StructType*>> classHierarchyMap;
	std::map<StructType*, std::vector<std::string>> classVirtualTableMap;
	std::set<StructType*> classAllocated;

private:
	std::map<std::string, std::set<std::string>> CHAResult;
	std::map<std::string, std::set<std::string>> RTAResult;
};
