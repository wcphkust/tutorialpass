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
#include "util/Dataflow.h"

using namespace llvm;
using namespace std;
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }


struct EnableFunctionOptPass : public FunctionPass {
    static char ID;
    EnableFunctionOptPass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override {
        if (F.hasFnAttribute(Attribute::OptimizeNone)) {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID = 0;

//------------------------------------------------------------------------
// Implementation of ProperSimulator, an instance of typestate analysis
//---------------------------------------------------------—--------------

/*
 * File state
 */
enum FileState {
    INIT,
    OPEN,
    CLOSE,
    ERROR
};

class FileVector{
public:
    FileState state;

public:
    FileVector() {
        state = INIT;
    }


};

class PropSim: public ModulePass,
               public DataFlow<FileVector>,
               public AssemblyAnnotationWriter{

public:
    
    static char ID;

    PropSim(): ModulePass(ID),DataFlow<FileVector>(true){
        
    };

    virtual void setBoundaryCondition(FileVector* blockBoundary){
       
    };

    virtual void meetOp(FileVector *lhs, const FileVector* rhs){
       
    };

    virtual FileVector *initFlowValue(BasicBlock& b,SetType setType){
        
        return new FileVector();
    };

    virtual FileVector *transferFunc(BasicBlock& bb){
    
        return new FileVector();
    };

    virtual bool evalFunc(Function& F){
        return false;
    };

    virtual bool runOnModule(Module &M){

        for(auto& f:M){
            if (f.isDeclaration()){
                continue;
            }
            evalFunc(f);
        }
        return false;
    }
};

char PropSim::ID = 0;
RegisterPass<PropSim> X("propsim", "Property simulation analysis");

static cl::opt<std::string>
        InputFilename(cl::Positional, cl::desc("<filename>.bc"), cl::init(""));

int main(int argc,const char *argv[]){

    LLVMContext& Context=getGlobalContext();

    SMDiagnostic Err;

    cl::ParseCommandLineOptions(argc, argv, "Property Simulation analysis...\n");

    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);

    if (!M) {
        Err.print(argv[0], errs());
        return -1;
    }

    llvm::legacy::PassManager Passes;
    Passes.add(new EnableFunctionOptPass());

    /// Transform it to SSA
    //  Passes.add(llvm::createPromoteMemoryToRegisterPass());
    //  Passes.add(new LoopInfoWrapperPass());

    Passes.add(new PropSim());
    Passes.run(*M.get());
    return 0;
}