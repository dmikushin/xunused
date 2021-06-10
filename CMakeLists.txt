cmake_minimum_required(VERSION 3.16)
project(xunused)

find_package(LLVM CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Found CLANG ${CLANG_PACKAGE_VERSION}")
message(STATUS "Using CLANGConfig.cmake in: ${CLANG_DIR}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(xunused main.cpp)

target_precompile_headers(xunused PRIVATE "pch.h")

target_link_directories(xunused
  PRIVATE
  ${LLVM_LIBRARY_DIRS}
)

target_compile_definitions(xunused
  PRIVATE
  ${LLVM_DEFINITIONS}
  "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS"
)
target_compile_options(xunused
  PRIVATE
  $<$<BOOL:${MSVC}>:/wd4624>
  $<$<BOOL:${MSVC}>:/wd4291>
)

target_include_directories(xunused
  PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${CLANG_INCLUDE_DIRS}
)

target_compile_features(xunused
  PRIVATE
  "cxx_std_17"
)

target_link_libraries(xunused
  PRIVATE
  clangIndex
  clangTooling
  clangFrontend
  clangDriver
  clangSerialization
  clangParse
  clangSema
  clangAnalysis
  clangASTMatchers
  clangAST
  clangEdit
  clangLex
  clangBasic
)

llvm_map_components_to_libnames(llvm_libs x86asmparser support option profiledata FrontendOpenMP)
target_link_libraries(xunused PRIVATE ${llvm_libs})

install(TARGETS xunused DESTINATION bin)
