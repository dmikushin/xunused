#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Tooling/AllTUsExecution.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Signals.h"
#include <map>
#include <memory>
#include <mutex>

using namespace clang;
using namespace clang::ast_matchers;

class XUnusedASTConsumer : public ASTConsumer {
public:
  XUnusedASTConsumer() {}
  void HandleTranslationUnit(ASTContext &Context) override {}
  bool HandleTopLevelDecl(DeclGroupRef D) override { return true; }
};

// For each source file provided to the tool, a new FrontendAction is created.
class XUnusedFrontendAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance & /*CI*/,
                                                 StringRef /*File*/) override {
    return std::make_unique<XUnusedASTConsumer>();
  }
};

class XUnusedFrontendActionFactory : public tooling::FrontendActionFactory {
public:
  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<XUnusedFrontendAction>();
  }
};

int main(int argc, const char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

  const char *Overview = R"(
  xunused is tool to find unused functions and methods across a whole C/C++ project.
  )";

  tooling::ExecutorName.setInitialValue("all-TUs");

  auto Executor = clang::tooling::createExecutorFromCommandLineArgs(
      argc, argv, llvm::cl::GeneralCategory, Overview);
  if (!Executor) {
    llvm::errs() << llvm::toString(Executor.takeError()) << "\n";
    return 1;
  }
  auto Err = Executor->get()->execute(
      std::make_unique<XUnusedFrontendActionFactory>());

  if (Err) {
    llvm::errs() << llvm::toString(std::move(Err)) << "\n";
  }
}
