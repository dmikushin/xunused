#ifndef MATCHER_H
#define MATCHER_H

#include "clang/Tooling/Tooling.h"

#include <map>
#include <string>
#include <vector>

struct DeclLoc
{
	DeclLoc() = default;

	DeclLoc(std::string Filename, unsigned Line);

	clang::SmallString<128> Filename;
	unsigned Line;
};

struct DefInfo
{
	const clang::FunctionDecl* definition;
	size_t uses;
	std::string name;
	std::string nameMangled;
	std::string filename;
	unsigned line;
	std::vector<DeclLoc> declarations;
};

extern std::map<std::string, DefInfo> g_allDecls;

std::unique_ptr<clang::tooling::FrontendActionFactory> createXUnusedFrontendActionFactory();

#endif // MATCHER_H

