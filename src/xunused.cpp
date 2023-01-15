#include "xunused.h"
#include "matcher.h"

#include "clang/Tooling/Tooling.h"
#include <llvm/DebugInfo/Symbolize/Symbolize.h>

#include <algorithm>
#include <execution>
#include <iostream>
#include <memory>

using namespace clang;
using namespace clang::tooling;
using namespace llvm::symbolize;

void xunused(CompilationDatabase& compilations,
	std::vector<UnusedDefInfo>& unused)
{
	auto&& sources = compilations.getAllFiles();
	const size_t total = std::size(sources);

	// TODO Must be re-iterated again and again, until no more unused functions are
	// included into the results.
	// TODO The checker must ensure the function is marked "used" only if all all
	// of its uses are used.
	std::for_each(std::execution::par_unseq, std::begin(sources), std::end(sources),
		[total, &sources, &compilations](auto && file)
	{
		int i = &file - &sources[0];
		static std::atomic_int32_t counter;
		std::cout << "[" << counter++ << "/" << total << "] " << file << std::endl;
		ClangTool Tool(compilations, file);
		Tool.run(createXUnusedFrontendActionFactory().get());
	});
	
	llvm::errs() << "Collected the following declarations:\n";
	for (auto & [USR, Decl] : g_defs)
		if (Decl.hasCode)
			llvm::errs() << LLVMSymbolizer::DemangleName(Decl.nameMangled, nullptr) << "\n";

	llvm::errs() << "Collected the following uses:\n";
	for (auto & [USR, Uses] : g_uses)
	{
		auto it = g_defs.find(USR);
		if (it == g_defs.end())
		{
			llvm::errs() << "Error: definition is missing for function '" << USR << "'\n";
			exit(-1); 
		}
		
		auto& Decl = it->second;
		if (!Decl.hasCode) continue;

		llvm::errs() << "Function '" << LLVMSymbolizer::DemangleName(Decl.nameMangled, nullptr) << "' is used in:\n";
		for (auto& Use : Uses)
		{
			auto it2 = g_defs.find(Use);
			if (it2 == g_defs.end())
			{
				llvm::errs() << "Error: definition is missing for user '" << USR << "'\n";
				exit(-1); 
			}
			
			auto& Decl2 = it2->second;		
			llvm::errs() << "\t" << LLVMSymbolizer::DemangleName(Decl2.nameMangled, nullptr) << "\n";
		}
	}
#if 0
	for (auto & [decl, I] : g_allDecls)
	{
		if (!I.definition) continue;
		if (I.uses != 0) continue;

		UnusedDefInfo def;
		def.name = I.name;
		def.nameMangled = I.nameMangled;
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
#endif
}

