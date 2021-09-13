//// Declares clang::SyntaxOnlyAction.
//#include "clang/Frontend/FrontendActions.h"
//#include "clang/Tooling/CommonOptionsParser.h"
//#include "clang/Tooling/Tooling.h"
//// Declares llvm::cl::extrahelp.
//#include "llvm/Support/CommandLine.h"
//
//using namespace clang::tooling;
//using namespace llvm;
//
//// Apply a custom category to all command-line options so that they are the
//// only ones displayed.
//static llvm::cl::OptionCategory MyToolCategory("my-tool options");
//
//// CommonOptionsParser declares HelpMessage with a description of the common
//// command-line options related to the compilation database and input files.
//// It's nice to have this help message in all tools.
//static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
//
//// A help message for this specific tool can be added afterwards.
//static cl::extrahelp MoreHelp("\nMore help text...\n");
//
//int main(int argc, const char ** argv)
//{
//  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory, cl::NumOccurrencesFlag::Optional);
//  if (!ExpectedParser)
//  {
//    // Fail gracefully for unsupported options.
//    llvm::errs() << ExpectedParser.takeError();
//    return 1;
//  }
//  CommonOptionsParser & OptionsParser = ExpectedParser.get();
//  ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
//  return Tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
//}

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"


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
using namespace llvm;
using namespace clang::ast_matchers;
using namespace clang::tooling;


class XUnusedASTConsumer : public ASTConsumer
{
public:
  XUnusedASTConsumer() {}
  void HandleTranslationUnit(ASTContext & Context) override {}
  bool HandleTopLevelDecl(DeclGroupRef D) override { return true; }
};

// For each source file provided to the tool, a new FrontendAction is created.
class XUnusedFrontendAction : public ASTFrontendAction
{
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance & /*CI*/, StringRef /*File*/) override
  {
    return std::make_unique<XUnusedASTConsumer>();
  }
};

class XUnusedFrontendActionFactory : public tooling::FrontendActionFactory
{
public:
  std::unique_ptr<FrontendAction> create() override { return std::make_unique<XUnusedFrontendAction>(); }
};

//int main(int argc, const char ** argv)
//{
//  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
//
//  const char * Overview = R"(
//  xunused is tool to find unused functions and methods across a whole C/C++ project.
//  )";
//
//  tooling::ExecutorName.setInitialValue("all-TUs");
//
//  auto Executor = clang::tooling::createExecutorFromCommandLineArgs(argc, argv, llvm::cl::GeneralCategory, Overview);
//  if (!Executor)
//  {
//    llvm::errs() << llvm::toString(Executor.takeError()) << "\n";
//    return 1;
//  }
//  auto Err = Executor->get()->execute(std::make_unique<XUnusedFrontendActionFactory>());
//
//  if (Err)
//  {
//    llvm::errs() << llvm::toString(std::move(Err)) << "\n";
//  }
//}

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char ** argv)
{
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory, cl::NumOccurrencesFlag::Optional);
  if (!ExpectedParser)
  {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  
  for (auto && file : ExpectedParser.get().getCompilations().getAllFiles())
  {
  }

  CommonOptionsParser & OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
  ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
  //return Tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
  return Tool.run(newFrontendActionFactory<XUnusedFrontendAction>().get());
}
