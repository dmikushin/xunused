struct Foo
{
  void OnBar()
  {

  }
};

int main(int argc, char const *argv[])
{
  using type = void (Foo::*)();
  type map [] =
  {
    static_cast<type>(&Foo::OnBar)
  };
  
  // auto ptr2 = &Foo::OnBar;
}

// m declRefExpr(isExpansionInMainFile()).bind("declRef")
// clang-query 'Source\3D\K3dDoc04\D3vedit.cpp'
