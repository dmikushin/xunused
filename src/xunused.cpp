#include "xunused.h"

#include "clang/Tooling/Tooling.h"

#include <algorithm>
#include <execution>
#include <iostream>
#include <memory>

using namespace clang;
using namespace clang::tooling;

std::unique_ptr<tooling::FrontendActionFactory> createXUnusedFrontendActionFactory();
void finalize();

void xunused(CompilationDatabase& compilations)
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

	finalize();
}

