#include "matcher.h"

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Mangle.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Tooling/AllTUsExecution.h"
#include <memory>
#include <mutex>
#include <map>

using namespace clang;
using namespace clang::ast_matchers;

static std::string getMangledName(const FunctionDecl* decl)
{
	auto& context = decl->getASTContext();
	auto mangleContext = context.createMangleContext();

	if (!mangleContext->shouldMangleDeclName(decl))
		return decl->getNameInfo().getName().getAsString();

	std::string mangledName;
	llvm::raw_string_ostream ostream(mangledName);

	mangleContext->mangleName(decl, ostream);

	ostream.flush();

	delete mangleContext;

	return mangledName;
};

template<class T, class Comp, class Alloc, class Predicate>
void discard_if(std::set<T, Comp, Alloc> & c, Predicate pred)
{
	for (auto it{c.begin()}, end{c.end()}; it != end; )
	{
		if (pred(*it))
			it = c.erase(it);
		else
			++it;
	}
}

DeclLoc::DeclLoc(std::string Filename, unsigned Line) :
	Filename(std::move(Filename)), Line(Line) { }

static std::mutex g_mutex;
std::map<std::string, DefInfo> g_allDecls;

bool getUSRForDecl(const Decl * decl, std::string & USR)
{
	llvm::SmallVector<char, 128> buff;

	if (index::generateUSRForDecl(decl, buff))
		return false;

	USR = std::string(buff.data(), buff.size());
	return true;
}

/// Returns all declarations that are not the definition of F
std::vector<DeclLoc> getDeclarations(const FunctionDecl * F, const SourceManager & SM)
{
	std::vector<DeclLoc> decls;
	for (const FunctionDecl * r : F->redecls())
	{
		if (r->doesThisDeclarationHaveABody())
			continue;

		auto begin = r->getSourceRange().getBegin();
		decls.emplace_back(SM.getFilename(begin).str(), SM.getSpellingLineNumber(begin));
		SM.getFileManager().makeAbsolutePath(decls.back().Filename);
	}
	return decls;
}

class FunctionDeclMatchHandler : public MatchFinder::MatchCallback
{
public :

	void finalize(const SourceManager & SM)
	{
		// Do this function excusively for each source file.
		// Note this lock must persist for the entire duration of finalization step
		// for the following reason. Each time FunctionDecls are only valid wrt
		// the pass working on the current source file. Once the next file finalization
		// is started, FunctionDecls are not valid anymore.
		std::unique_lock<std::mutex> lockGuard(g_mutex);
		
		// Unused definitions are FunctionDecl definitions excluding FunctionDecl uses.
		std::vector<const FunctionDecl *> unusedDefs;
		std::set_difference(_defs.begin(), _defs.end(),
			_uses.begin(), _uses.end(), std::back_inserter(unusedDefs));

		// Record each unused definition into our database.
		for (auto * F : unusedDefs)
		{
			F = F->getDefinition();
			assert(F);
			
			// Get USR (Unified Symbol Resolution) -
			// an unambiguous string identification of a symbol.
			std::string USR;
			if (!getUSRForDecl(F, USR))
				continue;
			
			auto&& [it, is_inserted] = g_allDecls.emplace(std::move(USR), DefInfo{F, 0});

			// If already inserted by a pass working on another source file,
			// its F pointer was only valid for it, and is not valid for the
			// current pass execution anymore. That's why we update the F
			// pointer to the value actual for the current pass.
			if (!is_inserted) it->second.definition = F;

			// Fill the contents of new record.
			auto Begin = F->getSourceRange().getBegin();
			it->second.filename = SM.getFilename(Begin).str();
			it->second.line = SM.getSpellingLineNumber(Begin);
			it->second.name = F->getQualifiedNameAsString();
			it->second.nameMangled = getMangledName(F);
			it->second.declarations = getDeclarations(F, SM);
		}

		// Weak functions are not the definitive definition. Remove it from
		// Defs before checking which uses we need to consider in other TUs,
		// so the functions overwritting the weak definition here are marked
		// as used.
		discard_if(_defs, [](const FunctionDecl * FD) { return FD->isWeak(); });

		std::vector<const FunctionDecl *> externalUses;

		// Uses excluded definitions are external uses: function is used in
		// the current source file, but is defined elsewhere.
		std::set_difference(_uses.begin(), _uses.end(),
			_defs.begin(), _defs.end(), std::back_inserter(externalUses));
#if 0
		for (auto *F : Uses)
			llvm::errs() << "Uses: " << F << " " << F->getNameAsString() << "\n";
		for (auto *F : Defs)
			llvm::errs() << "Defs: " << F << " " << F->getNameAsString() << "\n";
#endif
		for (auto* F : externalUses)
		{
			// llvm::errs() << "ExternalUses: " << F->getNameAsString() << "\n";
			std::string USR;
			if (!getUSRForDecl(F, USR)) continue;
			
			// Record external use as a function which is being used at least once.
			auto && [it, is_inserted] = g_allDecls.emplace(std::move(USR), DefInfo{nullptr, 1});
			
			// If the function is already recordered, possibly as unused, do make
			// sure it now becomes used.
			if (!is_inserted) it->second.uses++;
		}
		
		// TODO Find all uses of each function.
		// TODO If all known uses are unused, make this function unused.
		// TODO Make a graph of uses?
		
		// 1) Find each F tat has no uses
		// 2) Find all uses of F: If all uses are unused, make F unused.
	}

	void handleUse(const ValueDecl * D, const SourceManager * SM)
	{
		if (auto * FD = dyn_cast<FunctionDecl>(D))
		{
			// TODO
			// don't see afx BEGIN_MESSAGE_MAP
			//if (SM->isInSystemHeader(FD->getSourceRange().getBegin()))
			//return;

			if (FD->isTemplateInstantiation())
			{
				FD = FD->getTemplateInstantiationPattern();
				assert(FD);
			}

			_uses.insert(FD->getCanonicalDecl());
		}
	}

	void run(const MatchFinder::MatchResult & Result) override
	{
		if (const auto * F = Result.Nodes.getNodeAs<FunctionDecl>("fnDecl"))
		{
			if (!F->hasBody())
				return; // Ignore '= delete' and '= default' definitions.

			//if (F->isStatic())
			//return;

			// skip
			if (F->hasAttr<clang::DLLExportAttr>())
				return;

			if (F->isExternC())
				return;

			if (auto * templ = F->getInstantiatedFromMemberFunction())
				F = templ;

			if (F->isTemplateInstantiation())
			{
				F = F->getTemplateInstantiationPattern();
				assert(F);
			}

			auto begin = F->getSourceRange().getBegin();
			if (Result.SourceManager->isInSystemHeader(begin))
				return;

			if (!Result.SourceManager->isWrittenInMainFile(begin))
				return;

			if (auto * MD = dyn_cast<CXXMethodDecl>(F))
			{
				if (MD->isVirtual())
					return; // skip all virtual

				if (MD->isVirtual() && !MD->isPure() && MD->size_overridden_methods())
					return; // overriding method

				//if (isa<CXXConstructorDecl>(MD))
				//  return; // We don't see uses of constructors.

				if (isa<CXXDestructorDecl>(MD))
					return; // We don't see uses of destructors.
			}

			if (F->isMain())
				return;

			_defs.insert(F->getCanonicalDecl());

			// __attribute__((constructor())) are always used
			if (F->hasAttr<ConstructorAttr>())
				handleUse(F, Result.SourceManager);
		}
		else if (const auto * R = Result.Nodes.getNodeAs<DeclRefExpr>("declRef"))
		{
			handleUse(R->getDecl(), Result.SourceManager);
		}
		else if (const auto * R = Result.Nodes.getNodeAs<MemberExpr>("memberRef"))
		{
			handleUse(R->getMemberDecl(), Result.SourceManager);
		}
		else if (const auto * R = Result.Nodes.getNodeAs<CXXConstructExpr>("cxxConstructExpr"))
		{
			handleUse(R->getConstructor(), Result.SourceManager);
		}
	}

private :

	std::set<const FunctionDecl *> _defs;
	std::set<const FunctionDecl *> _uses;
};

class XUnusedASTConsumer : public ASTConsumer
{
public :

	XUnusedASTConsumer()
	{
		_matcher.addMatcher(functionDecl(isDefinition(), unless(isImplicit())).bind("fnDecl"), &_handler);
		_matcher.addMatcher(declRefExpr().bind("declRef"), &_handler);
		_matcher.addMatcher(memberExpr().bind("memberRef"), &_handler);
		_matcher.addMatcher(cxxConstructExpr().bind("cxxConstructExpr"), &_handler);
	}

	void HandleTranslationUnit(ASTContext & Context) override
	{
		_matcher.matchAST(Context);
		_handler.finalize(Context.getSourceManager());
	}

private :

	FunctionDeclMatchHandler _handler;
	MatchFinder _matcher;
};

// For each source file provided to the tool, a new FrontendAction is created.
class XUnusedFrontendAction : public ASTFrontendAction
{
public :

	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance & /*CI*/, StringRef /*File*/) override
	{
		return std::make_unique<XUnusedASTConsumer>();
	}
};

class XUnusedFrontendActionFactory : public tooling::FrontendActionFactory
{
public :

	std::unique_ptr<FrontendAction> create() override
	{
		return std::make_unique<XUnusedFrontendAction>();
	}
};

std::unique_ptr<tooling::FrontendActionFactory> createXUnusedFrontendActionFactory()
{
	return std::make_unique<XUnusedFrontendActionFactory>();
}

