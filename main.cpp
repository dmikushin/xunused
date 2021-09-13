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

#include <algorithm>
#include <execution>
#include <iostream>

using namespace clang;
using namespace llvm;
using namespace clang::ast_matchers;
using namespace clang::tooling;


static llvm::cl::OptionCategory MyToolCategory("my-tool options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...\n");


std::unique_ptr<tooling::FrontendActionFactory> createXUnusedFrontendActionFactory();
void finalize();


int main(int argc, const char ** argv)
{
  auto expectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory, cl::NumOccurrencesFlag::ZeroOrMore);
  if (!expectedParser)
  {
    // Fail gracefully for unsupported options.
    llvm::errs() << expectedParser.takeError();
    return 1;
  }

  auto & optionsParser = expectedParser.get();

  const size_t limit = 9999;
  size_t i = 0;

  std::vector<std::function<void()>> tasks;

  for (auto && file : optionsParser.getCompilations().getAllFiles())
  //for (auto && file : optionsParser.getSourcePathList())
  {
    if (++i > limit)
      break;

    tasks.emplace_back(
      [i, file, &optionsParser]
      {
        std::cout << i << " " << file << std::endl;
        ClangTool Tool(optionsParser.getCompilations(), file);
        Tool.run(createXUnusedFrontendActionFactory().get());
      });
  }

  std::for_each(std::execution::par_unseq, std::begin(tasks), std::end(tasks), [](auto && f) { f(); });

  finalize();


  return 0;
}
