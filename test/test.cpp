// extern "C" void foo()
// {
// }

[[gnu::used]]
void bar()
{
}

__declspec(dllexport)
void foo()
{
}

// int main(int argc, char const *argv[])
// {
// }

// m declRefExpr(isExpansionInMainFile()).bind("declRef")
// clang-query 'Source\3D\K3dDoc04\D3vedit.cpp'

// xunused 3D\K3dDoc04\D3vedit.cpp


// set bind-root true
// set output detailed-ast
// m functionDecl( hasAttr("attr::Used") )
// m functionDecl( unless(hasAttr("attr::Used")) ) 
// m functionDecl( unless(hasAttr("attr::DLLExport")) ) 
// m functionDecl( unless(hasAttr("attr::DLLImport")) )