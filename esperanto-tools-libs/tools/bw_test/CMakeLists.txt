# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.17)

project(bwTest VERSION 0.1.0)
enable_language(CXX ASM)
find_package(OpenMP)
add_executable(bwTest bwTest.cpp asm_memcpy.S)
target_compile_features(bwTest PRIVATE cxx_std_17)
target_link_libraries(bwTest PUBLIC OpenMP::OpenMP_CXX)



