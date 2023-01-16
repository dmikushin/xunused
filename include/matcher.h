#ifndef MATCHER_H
#define MATCHER_H

#include "clang/Tooling/Tooling.h"

#include <map>
#include <set>
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
	std::string filename;
	unsigned line;
	std::string name;
	std::string nameMangled;
	bool hasBody;
	bool isImplicit;
	bool used = true;
	bool alwaysUsed = false;
	std::set<std::string> uses;
	std::map<std::string, DeclLoc> decls;
};

extern std::map<std::string, DefInfo> g_defs;
extern std::map<std::string, std::set<std::string> > g_uses;

std::unique_ptr<clang::tooling::FrontendActionFactory> createXUnusedFrontendActionFactory();

#endif // MATCHER_H

