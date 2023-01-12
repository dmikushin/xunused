cmake_minimum_required(VERSION 3.16)
project(xunused)

find_package(LLVM CONFIG REQUIRED PATHS "/usr/lib/llvm-12/lib/cmake" NO_DEFAULT_PATH)
find_package(Clang CONFIG REQUIRED PATHS "/usr/lib/llvm-12/lib/cmake" NO_DEFAULT_PATH)

# Make sure we pick up the system-provided TBB, which is compatible with GCC,
# and not the idiotic oneAPI, which broke everything.
find_package(TBB PATHS "/usr/lib/x86_64-linux-gnu/cmake/TBB" REQUIRED NO_DEFAULT_PATH)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Make a static library to use in other projects as well.
add_library(${PROJECT_NAME}_embedded STATIC "src/matcher.cpp" "src/xunused.cpp")
target_include_directories(${PROJECT_NAME}_embedded PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME}_embedded PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_compile_definitions(${PROJECT_NAME}_embedded PRIVATE ${LLVM_DEFINITIONS} _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
target_compile_features(${PROJECT_NAME}_embedded PRIVATE cxx_std_17)
target_link_directories(${PROJECT_NAME}_embedded PRIVATE ${LLVM_LIBRARY_DIRS})
target_compile_options(${PROJECT_NAME}_embedded PRIVATE $<$<BOOL:${MSVC}>:/wd4624> $<$<BOOL:${MSVC}>:/wd4291> $<$<BOOL:${MSVC}>:/MT>)
target_link_libraries(${PROJECT_NAME}_embedded PRIVATE clangIndex clangTooling clangFrontend clangDriver clangSerialization
	clangParse clangSema clangAnalysis clangASTMatchers clangAST clangEdit clangLex clangBasic TBB::tbb)

add_executable(${PROJECT_NAME} "src/main.cpp")
target_include_directories(${PROJECT_NAME} PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_embedded)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

