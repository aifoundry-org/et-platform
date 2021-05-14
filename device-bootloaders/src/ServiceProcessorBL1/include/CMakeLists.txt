#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

get_filename_component(GET_GIT_HASH_ABS_PATH "${ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR}/esperanto-fw/get_git_hash.py" ABSOLUTE)
get_filename_component(GET_GIT_VERSION_ABS_PATH "${ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR}/esperanto-fw/get_git_version.py" ABSOLUTE)
set(CMAKE_GET_GIT_HASH ${GET_GIT_HASH_ABS_PATH}                  CACHE PATH   "get-git-hash" FORCE)
set(CMAKE_GET_GIT_VERSION ${GET_GIT_VERSION_ABS_PATH}            CACHE PATH   "get-git-version" FORCE)

if (NOT DEFINED GIT_HASH_STRING)
  execute_process(COMMAND ${CMAKE_GET_GIT_HASH} ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_HASH_STRING RESULT_VARIABLE RES)
  if (RES)
    message(FATAL_ERROR "Get git hash string failed with: ${OUTPUT_VARIABLE}")
  endif()
endif()
if (NOT DEFINED GIT_HASH_ARRAY)
  execute_process(COMMAND ${CMAKE_GET_GIT_HASH} -a ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_HASH_ARRAY RESULT_VARIABLE RES)
  if (RES)
    message(FATAL_ERROR "Get git hash array string failed with: ${OUTPUT_VARIABLE}")
  endif()
endif()
if (NOT DEFINED GIT_VERSION_STRING)
  execute_process(COMMAND ${CMAKE_GET_GIT_VERSION} ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_VERSION_STRING RESULT_VARIABLE RES)
  if (RES)
    message(FATAL_ERROR "Get git version failed with: ${OUTPUT_VARIABLE}")
  endif()
endif()
if (NOT DEFINED GIT_VERSION_ARRAY)
  execute_process(COMMAND ${CMAKE_GET_GIT_VERSION} -a ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_VERSION_ARRAY RESULT_VARIABLE RES)
  if (RES)
    message(FATAL_ERROR "Get git version array string failed with: ${OUTPUT_VARIABLE}")
  endif()
endif()

configure_file(build_configuration.h.in build_configuration.h @ONLY)
