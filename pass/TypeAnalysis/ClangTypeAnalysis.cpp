#include <iostream>
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

using namespace clang;
using namespace ento;

namespace {
    class ClangTypeAnalysis : public Checker < check::PreStmt<Stmt>,
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

    ST->dumpPretty(C.getASTContext());

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

