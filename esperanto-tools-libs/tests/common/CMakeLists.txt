#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(EsperantoRuntime_VERSION_MAJOR ${CMAKE_MAJOR_VERSION})
set(EsperantoRuntime_VERSION_MINOR ${CMAKE_MINOR_VERSION})
set(EsperantoRuntime_VERSION_PATCH ${CMAKE_PATCH_VERSION})

execute_process(
    COMMAND git rev-parse --verify HEAD
    OUTPUT_VARIABLE ET_RUNTIME_GIT_HASH
    TIMEOUT 20
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

get_target_property(ETRT_OBJ_INCLUDE_DIRS etrt_obj INCLUDE_DIRECTORIES)
add_library(test_utils MpOrchestrator.cpp)
add_library(runtime::test_utils ALIAS test_utils)
target_include_directories(test_utils 
INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
PRIVATE ${ETRT_OBJ_INCLUDE_DIRS}
)

target_link_libraries(test_utils PRIVATE runtime::etrt_static cereal::cereal deviceLayer::deviceLayer)
configure_file(Constants.h.in Constants.h)
