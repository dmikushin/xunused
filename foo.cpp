#include "foo.hpp"

int foo()
{}

  void bar::bas()
  {
    auto ref = &::foo;
  }

int main(int argc, char const *argv[])
{
  bar bar;
  bar.bas();
  return 0;
}
