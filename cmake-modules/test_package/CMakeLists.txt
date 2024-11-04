cmake_minimum_required(VERSION 3.1)
project(CmakeMoulesTest CXX)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

option(ENABLE_IPO                  "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" ON)
option(ENABLE_UNITY                "Enable Unity builds of projects" OFF)
include(CommonProjectSettings)

option(ENABLE_COMPILER_CACHE       "Enable compiler cache (ccache/distcc) if available" OFF)
include(CompilerCache)

option(ENABLE_EXCEPTIONS           "Enable exceptions" ON)
option(ENABLE_RTTI                 "Enable RTTI" ON)
option(ENABLE_PCH                  "Enable Precompiled Headers" OFF)
include(CompilerOptions)

option(WARNINGS_AS_ERRORS          "Treat compiler warnings as errors" ON)
include(CompilerWarnings)

option(ENABLE_TESTING              "Enable test builds" ON)
option(ENABLE_COVERAGE             "Enable coverage reporting for gcc/clang" ON)
option(ENABLE_SANITIZER_ADDRESS    "Enable address sanitizer" OFF)
option(ENABLE_SANITIZER_LEAK       "Enable leak sanitizer" OFF)
option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" OFF)
option(ENABLE_SANITIZER_THREAD     "Enable thread sanitizer" OFF)
option(ENABLE_SANITIZER_MEMORY     "Enable memory sanitizer" OFF)
include(CompilerSanitizers)

option(ENABLE_CPPCHECK             "Enable static analysis with cppcheck" OFF)
option(ENABLE_CLANG_TIDY           "Enable static analysis with clang-tidy" OFF)
option(ENABLE_INCLUDE_WHAT_YOU_USE "Enable static analysis with include-what-you-use" OFF)
option(ENABLE_CPPLINT              "Enable static analysis with cpplint" OFF)
include(StaticAnalyzers)

add_executable(test_package test_executable.cpp)
target_set_project_options(test_package)
target_set_project_warnings(test_package)
target_add_sanitizers(test_package)
target_link_libraries(test_package PRIVATE ${CONAN_LIBS})
