#include "xunused.h"
#include "matcher.h"

#include "clang/Tooling/Tooling.h"

#include <algorithm>
#include <execution>
#include <iostream>
#include <memory>

using namespace clang;
using namespace clang::tooling;

void xunused(CompilationDatabase& compilations,
	std::vector<UnusedDefInfo>& unused)
{
	auto&& sources = compilations.getAllFiles();
	const size_t total = std::size(sources);

	std::for_each(std::execution::par_unseq, std::begin(sources), std::end(sources),
		[total, &sources, &compilations](auto && file)
	{
		int i = &file - &sources[0];
		static std::atomic_int32_t counter;
		std::cout << "[" << counter++ << "/" << total << "] " << file << std::endl;
		ClangTool Tool(compilations, file);
		Tool.run(createXUnusedFrontendActionFactory().get());
	});

	for (auto & [decl, I] : g_allDecls)
	{
		if (!I.definition) continue;
		if (I.uses != 0) continue;

		UnusedDefInfo def;
		def.name = I.name;
		def.filename = I.filename;
		def.line = I.line;
		def.declarations.resize(I.declarations.size());
		for (int i = 0; i < def.declarations.size(); i++)
		{
			auto& decl = def.declarations[i];
			decl.filename = I.declarations[i].Filename.str();
			decl.line = I.declarations[i].Line;
		}
		
		unused.push_back(def);
	}
}

