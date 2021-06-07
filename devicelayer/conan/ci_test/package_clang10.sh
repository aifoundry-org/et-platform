#!/bin/bash

set -e
set -x

# export CONAN_CONFIG_URL="git@gitlab.esperanto.ai:software/conan/conan-config.git"
export CONAN_ARCHS="x86_64"
export CONAN_OPTIONS="g3log:use_dynamic_logging_levels=True,boost:header_only=True"
export CONAN_CPPSTDS="17"
export CONAN_BUILD_POLICY="missing"
export CONAN_CMAKE_GENERATOR="Ninja"
export CONAN_CLANG_VERSIONS="10"

conan user
python3 conan/package.py

unset CONAN_CONFIG_URL
unset CONAN_ARCHS
unset CONAN_OPTIONS
unset CONAN_CPPSTDS
unset CONAN_BUILD_POLICY
unset CONAN_CMAKE_GENERATOR
unset CONAN_CLANG_VERSIONS
