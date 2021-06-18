#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Copyright (c) Glow Contributors. See CONTRIBUTORS file.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

option(USE_COVERAGE "Define whether coverage report needs to be generated" OFF)

if(USE_COVERAGE)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Code coverage must be run in debug mode, otherwise it might be misleading.")
  endif()

  find_program(GCOV_PATH gcov)
  if(NOT GCOV_PATH)
    message(FATAL_ERROR "Make sure gcov is installed.")
  endif()

  find_program(LCOV_PATH NAMES lcov lcov.bat lcov.exe lcov.perl)
  if(NOT LCOV_PATH)
    message(FATAL_ERROR "Make sure lcov is installed.")
  endif()

  find_program(GENHTML_PATH NAMES genhtml genhtml.perl genhtml.bat)
  if(NOT GENHTML_PATH)
    message(FATAL_ERROR "Make sure genhtml is installed.")
  endif()

  # Add compilation flags for coverage.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")

  # Add coverage target.
  add_custom_target(coverage
    COMMENT "Starting coverage analysis"

    # Cleanup lcov counters.
    COMMAND ${LCOV_PATH} --directory . --zerocounters
    COMMAND echo "Cleaning is done. Running tests"

    # Run all tests.
    COMMAND ${CMAKE_CTEST_COMMAND} -j 4 -L Generic

    # Capture lcov counters based on the test run.
    COMMAND ${LCOV_PATH} --no-checksum --directory . --capture --output-file coverage.info

    # Ignore not related files.
    COMMAND ${LCOV_PATH} --remove coverage.info '*v1*' '/usr/*' '/opt/*' '*sw-platform-x86_64-sysroot/*' '*tests/*' '*llvm_install*' 'build/*' --output-file ${PROJECT_BINARY_DIR}/coverage_result.info

    # Generate HTML report based on the profiles.
    COMMAND ${GENHTML_PATH} -o coverage ${PROJECT_BINARY_DIR}/coverage_result.info

    # Cleanup info files.
    COMMAND ${CMAKE_COMMAND} -E remove coverage.info ${PROJECT_BINARY_DIR}/coverage_result.info

    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )

  add_custom_command(TARGET coverage POST_BUILD
    COMMAND ;
    COMMENT "Coverage report: ./coverage/index.html. Open it in your browser."
  )
endif()
