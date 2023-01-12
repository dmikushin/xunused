#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Tooling/AllTUsExecution.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
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

static const char usageText[] = "";

std::unique_ptr<tooling::FrontendActionFactory> createXUnusedFrontendActionFactory();
void finalize();

int main(int argc, const char ** argv)
{
	CommonOptionsParser optionsParser(argc, argv, MyToolCategory, usageText);

	auto&& sources = optionsParser.getCompilations().getAllFiles();
	const size_t total = std::size(sources);

	std::for_each(std::execution::par_unseq, std::begin(sources), std::end(sources),
		[total, &sources, &optionsParser](auto && file)
	{
		int i = &file - &sources[0];
		static std::atomic_int32_t counter;
		std::cout << "[" << counter++ << "/" << total << "] " << file << std::endl;
		ClangTool Tool(optionsParser.getCompilations(), file);
		Tool.run(createXUnusedFrontendActionFactory().get());
	});

	finalize();

	return 0;
}

