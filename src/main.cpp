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

	std::vector<UnusedDefInfo> unused;
	xunused(optionsParser.getCompilations(), unused);

	for (int i = 0; i < unused.size(); i++)
	{
		auto I = unused[i];
 
		llvm::errs() << I.filename << ":" << I.line << ": warning:" <<
			" Function '" << I.name << "' is unused";
		for (auto & D : I.declarations)
			llvm::errs() << " " << D.filename << ":" << D.line <<
				": note:" << " declared here";

		llvm::errs() << "\n";
	}

	return 0;
}

