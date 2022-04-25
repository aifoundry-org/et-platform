#------------------------------------------------------------------------------
# Copyright (C) 2022, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.17)

project(bwTest VERSION 0.1.0)
enable_language(CXX ASM)
find_package(OpenMP)
add_executable(bwTest bwTest.cpp asm_memcpy.S)
target_compile_features(bwTest PRIVATE cxx_std_17)
target_link_libraries(bwTest PUBLIC OpenMP::OpenMP_CXX)



