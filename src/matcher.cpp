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

using namespace clang;
using namespace clang::ast_matchers;

static std::mutex g_mutex;
std::map<std::string, DefInfo> g_defs;
std::map<std::string, std::set<std::string> > g_uses;

DeclLoc::DeclLoc(std::string Filename, unsigned Line) :
	Filename(std::move(Filename)), Line(Line) { }

// Get mangled name for a declaration.
// https://stackoverflow.com/q/40740604/4063520
static std::string getMangledName(const FunctionDecl* decl)
{
	auto& context = decl->getASTContext();
	std::unique_ptr<MangleContext> mangleContext(context.createMangleContext());

	if (!mangleContext->shouldMangleDeclName(decl))
		return decl->getNameInfo().getName().getAsString();

	// Spent a day figuring out mangling a constructor needs
	// this crazy shit, see clang/lib/AST/Mangle.cpp.
	GlobalDecl GD;
	if (const auto *CtorD = dyn_cast<CXXConstructorDecl>(decl))
		GD = GlobalDecl(CtorD, Ctor_Complete);
	else if (const auto *DtorD = dyn_cast<CXXDestructorDecl>(decl))
		GD = GlobalDecl(DtorD, Dtor_Complete);
	else
		GD = GlobalDecl(decl);

	std::string mangledName;
	llvm::raw_string_ostream ostream(mangledName);

	mangleContext->mangleName(GD, ostream);

	ostream.flush();

	return mangledName;
}

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

static bool getUSRForDecl(const Decl * decl, std::string & USR)
{
	llvm::SmallVector<char, 128> buff;

	if (index::generateUSRForDecl(decl, buff))
		return false;

	USR = std::string(buff.data(), buff.size());
	return true;
}

/// Returns all declarations that are not the definition of F
static void appendDeclarations(const FunctionDecl * F, const SourceManager & SM,
	std::vector<DeclLoc>& decls)
{
	for (const FunctionDecl* R : F->redecls())
	{
		if (R->doesThisDeclarationHaveABody())
			continue;

		auto begin = R->getSourceRange().getBegin();
		decls.emplace_back(SM.getFilename(begin).str(),
			SM.getSpellingLineNumber(begin));
		SM.getFileManager().makeAbsolutePath(decls.back().Filename);
	}
}

class FunctionDeclMatchHandler : public MatchFinder::MatchCallback
{
	// Get the function, which uses the given match.
	// https://stackoverflow.com/a/72642060/4063520
	template<typename Match>
	const FunctionDecl* getMatchUser(Match FD)
	{
		clang::DynTypedNodeList NodeList = context->getParents(*FD);

		while (!NodeList.empty())
		{
			// Get the first parent.
			clang::DynTypedNode ParentNode = NodeList[0];

			// You can dump the parent like this to inspect it.
			//ParentNode.dump(llvm::outs(), Context);

			// Is the parent a FunctionDecl?
			if (const FunctionDecl *Parent = ParentNode.get<FunctionDecl>())
				return Parent;

			// It was not a FunctionDecl.  Keep going up.
			NodeList = context->getParents(ParentNode);
		}

		// Ran out of ancestors.
		return nullptr;
	}

public :

	void finalize(const SourceManager & SM)
	{
		// Do this function excusively for each source file.
		// Note this lock must persist for the entire duration of finalization step
		// for the following reason. Each time FunctionDecls are only valid wrt
		// the pass working on the current source file. Once the next file finalization
		// is started, FunctionDecls are not valid anymore.
		std::unique_lock<std::mutex> lockGuard(g_mutex);
		
		// Merge current functions uses into the global index.
		for (const auto& use : _uses)
		{
			const FunctionDecl* F = use.first;
			const std::set<const FunctionDecl *>& MoreUses = use.second;

			// Get signature of a function.
			std::string FSig;
			if (!getUSRForDecl(F, FSig)) return;

			// Make sure the function definition exists.
			{
				auto it = _defs.find(FSig);
				if (it == _defs.end())
					_defs[FSig] = F;
			}

			auto&& [it, is_inserted] = g_uses.emplace(std::move(FSig), std::set<std::string>());
			auto& Uses = it->second;
			for (const auto& U : MoreUses)
			{
				std::string USig;
				if (!getUSRForDecl(U, USig)) return;

				Uses.insert(USig);
			
				// Make sure all uses refer to existing definitions.
				auto it = _defs.find(USig);
				if (it == _defs.end())
					_defs[USig] = U;
			}
		}
		
		// Merge and expand current functions definitions into the global index.
		for (const auto& def : _defs)
		{
			const std::string& Sig = def.first;
			const FunctionDecl* F = def.second;

			if (!F) continue;

			F = F->getDefinition();
			if (!F) continue;

			// Extract function information for the global storage of definitions.
			auto&& [it, is_inserted] = g_defs.emplace(std::move(Sig), DefInfo());
			auto& FInfo = it->second;
			if (is_inserted)
			{
				const auto& Begin = F->getSourceRange().getBegin();
				FInfo.filename = SM.getFilename(Begin).str();
				FInfo.line = SM.getSpellingLineNumber(Begin);
				FInfo.name = F->getQualifiedNameAsString();
				FInfo.nameMangled = getMangledName(F);
				FInfo.hasBody = F->hasBody();
				FInfo.isImplicit = F->isImplicit();
				FInfo.alwaysUsed = F->isMain() || F->hasAttr<UsedAttr>() || F->hasAttr<DLLExportAttr>();
			}
			
			// Append non-accounted declarations of current def.
			appendDeclarations(F, SM, FInfo.decls);
		}
	}

	template<typename Match>
	void handleUse(Match R, const ValueDecl * D, const SourceManager * SM)
	{
		//D->dump(llvm::outs());

		auto* FD = dyn_cast<FunctionDecl>(D);
		if (!FD) return;

		if (FD->isTemplateInstantiation())
		{
			FD = FD->getTemplateInstantiationPattern();
			assert(FD);
		}

		// Get function, which uses the current match.
		const FunctionDecl* FDUser = getMatchUser(R);
		if (!FDUser) return;

		_uses[FD].insert(FDUser);
	}

	bool shouldHandleFunctionDecl(const FunctionDecl* F, const SourceManager * SM)
	{
		if (!F) return false;

		if (!F->hasBody())
			return false;
#if 0
		if (F->isExternC())
			return false;
#endif
		if (auto * templ = F->getInstantiatedFromMemberFunction())
			F = templ;

		if (F->isTemplateInstantiation())
		{
			F = F->getTemplateInstantiationPattern();
			assert(F);
		}

		auto begin = F->getSourceRange().getBegin();
		if (SM->isInSystemHeader(begin))
			return false;

		if (auto * MD = dyn_cast<CXXMethodDecl>(F))
		{
#if 0
			if (MD->isVirtual())
				return false; // skip all virtual

			if (MD->isVirtual() && !MD->isPure() && MD->size_overridden_methods())
				return false; // overriding method
#endif
#if 0
			if (isa<CXXConstructorDecl>(MD))
				return; // We don't see uses of constructors.
#endif
			if (isa<CXXDestructorDecl>(MD))
				return false; // We don't see uses of destructors.
		}

		if (F->isMain())
			return false;

		return true;
	}

	void run(const MatchFinder::MatchResult & Result) override
	{
		if (const auto * R = Result.Nodes.getNodeAs<FunctionDecl>("namedDecl"))
		{
			auto* F = R->getDefinition();

			if (shouldHandleFunctionDecl(F, Result.SourceManager))
				handleUse(R, F, Result.SourceManager);
		}
		else if (const auto * F = Result.Nodes.getNodeAs<FunctionDecl>("functionDecl"))
		{
			if (shouldHandleFunctionDecl(F, Result.SourceManager))
			{
				// Get signature of a function.
				std::string FSig;
				if (!getUSRForDecl(F, FSig)) return;

				_defs[FSig] = F->getCanonicalDecl();
#if 0
				// __attribute__((constructor())) are always used
				if (F->hasAttr<ConstructorAttr>())
					handleUse(F, Result.SourceManager);
#endif
			}
		}
		else if (const auto * R = Result.Nodes.getNodeAs<DeclRefExpr>("declRefExpr"))
		{
			handleUse(R, R->getDecl(), Result.SourceManager);
		}
		else if (const auto * R = Result.Nodes.getNodeAs<MemberExpr>("memberExpr"))
		{
			handleUse(R, R->getMemberDecl(), Result.SourceManager);
		}
		else if (const auto * R = Result.Nodes.getNodeAs<CXXConstructExpr>("cxxConstructExpr"))
		{
			handleUse(R, R->getConstructor(), Result.SourceManager);
		}
	}

	void setASTContext(ASTContext* context_)
	{
		context = context_;
	}

private :

	ASTContext* context = nullptr;

	std::map<std::string, const FunctionDecl *> _defs;
	std::map<const FunctionDecl *, std::set<const FunctionDecl *> > _uses;
};

class XUnusedASTConsumer : public ASTConsumer
{
public :

	XUnusedASTConsumer()
	{
		_matcher.addMatcher(namedDecl().bind("namedDecl"), &_handler);
		_matcher.addMatcher(functionDecl().bind("functionDecl"), &_handler);
		_matcher.addMatcher(declRefExpr().bind("declRefExpr"), &_handler);
		_matcher.addMatcher(memberExpr().bind("memberExpr"), &_handler);
		_matcher.addMatcher(cxxConstructExpr().bind("cxxConstructExpr"), &_handler);
	}

	void HandleTranslationUnit(ASTContext & Context) override
	{
		_handler.setASTContext(&Context);
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

