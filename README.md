# Find unused C/C++ functions with `xunused`

`xunused` is a tool to find unused C/C++ functions and methods across source files in the whole project.
It is built upon clang to parse the source code (in parallel). It then shows all functions that had
a definition but no use. Templates, virtual functions, constructors, functions with `static` linkage are
all taken into account.

`xunused` is compatible with LLVM and Clang version 12.

## Prerequisites

```
sudo apt install llvm-12-dev libclang-12-dev libtbb-dev
```

## Building

```
mkdir build
cd build
cmake ..
make
```

## Testing

To run the tool, provide a [compilation database](https://clang.llvm.org/docs/JSONCompilationDatabase.html). By default, it will analyze all files that are mentioned in it.

In order to generate `compile_commands.json` for a CMake project, add the following line into `CMakeLists.txt`:

```
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

An example `compile_commands.json` file is create in the `build` folder:

```
cd build
./xunused ./compile_commands.json
```

You can specify the option `-filter` together with a regular expressions. Only files who's path is matching the regular
expression will be analyzed. You might want to exclude your test's source code to find functions that are only used by tests but not any other code.

If `xunused` complains about missing include files such as `stddef.h`, try adding `-extra-arg=-I/usr/include/clang/10/include` (or similar) to the arguments.

