#ifndef XUNUSED_H
#define XUNUSED_H

#include "clang/Tooling/CompilationDatabase.h"

#include <string>
#include <vector>

struct UnusedDeclLoc
{
	std::string filename;
	unsigned line;
};

struct UnusedDefInfo
{
	std::string name;
	std::string filename;
	unsigned line;
	std::vector<UnusedDeclLoc> declarations;
};

void xunused(clang::tooling::CompilationDatabase& compilations,
	std::vector<UnusedDefInfo>& unused);

#endif // XUNUSED_H

