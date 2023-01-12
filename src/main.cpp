#include "xunused.h"

#include "clang/Driver/Options.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"

using namespace clang::tooling;

static llvm::cl::OptionCategory MyToolCategory("my-tool options");

static const char usageText[] = "";

int main(int argc, const char ** argv)
{
	CommonOptionsParser optionsParser(argc, argv, MyToolCategory, usageText);

	xunused(optionsParser.getCompilations());

	return 0;
}

