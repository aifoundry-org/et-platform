#------------------------------------------------------------------------------
# Copyright (C) 2020,2021 Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.17)
project(launchSim VERSION 0.1.0)

include_directories(${CMAKE_INSTALL_INCLUDEDIR}/device-layer)

add_executable(launchSim tf_helpers/tf_manager.cpp)

target_compile_features(launchSim PRIVATE cxx_std_17)
target_include_directories(launchSim PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(launchSim
    PRIVATE
        deviceLayer::deviceLayer
)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
    target_link_libraries(launchSim PUBLIC stdc++fs)
endif ()

install(DIRECTORY "tests"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
