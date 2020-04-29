#include <iostream>
#include <fstream>
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

using namespace clang;
using namespace ento;
using namespace std;

namespace {
    class ClangTypeAnalysis : public Checker <check::PreStmt<Stmt>,
                              check::ASTDecl<Decl>,
                              check::EndAnalysis > {
    mutable std::unique_ptr<BugType> BT;

    public:
    void checkPreStmt(const Stmt *ST, CheckerContext &C) const;
    void checkASTDecl(const Decl *D,
                      AnalysisManager &Mgr,
                      BugReporter &BR) const;
    void checkEndAnalysis(ExplodedGraph &G,
                          BugReporter &BR, ExprEngine &Eng) const;
};
} // end anonymous namespace

void ClangTypeAnalysis::checkPreStmt(const Stmt *ST, CheckerContext &C) const {
    SourceLocation Loc = ST->getBeginLoc();
    auto& SM = C.getSourceManager();

    if ((Loc.isInvalid() || SM.isInSystemHeader(Loc) || SM.isInExternCSystemHeader(Loc) || SM.isInSystemMacro(Loc)) && !SM.isWrittenInMainFile(Loc)) {
        return;
    }

    //Output the ast cont


//    ST->dumpPretty(C.getASTContext());

    // generate and solve the constraints
}

void ClangTypeAnalysis::checkASTDecl(const Decl *D,
                                     AnalysisManager &Mgr,
                                     BugReporter &BR) const {
    SourceLocation Loc = D->getBeginLoc();
    auto& SM = Mgr.getSourceManager();

    if ((Loc.isInvalid() || SM.isInSystemHeader(Loc) || SM.isInExternCSystemHeader(Loc) || SM.isInSystemMacro(Loc)) && !SM.isWrittenInMainFile(Loc)) {
        return;
    }

    // generate and solve the constraints
    if (auto functionDeclaration = dyn_cast<FunctionDecl>(D)) {    // Referenced from children
        int paraNum = functionDeclaration->getNumParams();
        cout << "[[" << functionDeclaration->getName().str() << "]]";

        cout << "(";
        for (int i = 0; i < paraNum; i++) {
            auto paraDecl = functionDeclaration->getParamDecl(i);
            QualType paraType = paraDecl->getType();

            cout << paraType.getAsString();
            if (i < paraNum - 1) {
                cout << ",";
            }
        }
        cout << ")";
        cout << "->";

        cout << functionDeclaration->getReturnType().getAsString() << endl;
    }

    if (auto variableDeclaration = dyn_cast<VarDecl>(D)) {
        string variableName = variableDeclaration->getName().str();

        QualType varType = variableDeclaration->getType();
        StringRef varName = variableDeclaration->getName();
        cout << "[[" << varName.str() << "]]"
             << "   " << varType.getAsString() << endl;
    }
}


void ClangTypeAnalysis::checkEndAnalysis(ExplodedGraph &G,
                                         BugReporter &BR, ExprEngine &Eng) const {
    // output the constraints
    

}

// Register plugin!
extern "C"
void clang_registerCheckers (CheckerRegistry &registry) {
    registry.addChecker<ClangTypeAnalysis>("homework.ClangTypeAnalysis", "Type Analysis", "nodoc");
}

extern "C"
const char clang_analyzerAPIVersionString[] = CLANG_ANALYZER_API_VERSION_STRING;

