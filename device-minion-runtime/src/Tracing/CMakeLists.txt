#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(DEVAPI_LIB ${ESPERANTO_DEVICE_API_LIB_DIR}/esperanto/device-apis)
set(DEVAPI_CODE_GEN ${DEVAPI_LIB}/device_apis_codegen.py)

# List of Jinja files to generate files for
SET(JINJA_FILES
  "device-mrt-trace.h.jinja:device-mrt-trace.h"
  "device-mrt-trace.c.jinja:device-mrt-trace.c"
  )

include_directories(${ESPERANTO_DEVICE_API_INCLUDE_DIR})
include_directories(${SHARED_INC_DIRS})

foreach(TMPL_TUPLE ${JINJA_FILES})
  string(REPLACE ":" ";" TMPL_LIST ${TMPL_TUPLE})
  list(GET TMPL_LIST 0 TMPL)
  list(GET TMPL_LIST 1 GEN_OUTPUT)
  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     COMMAND ${DEVAPI_CODE_GEN}
     ARGS  --spec ${DEVAPI_LIB}/operations-api/operations-api.yaml
           --schema ${DEVAPI_LIB}/schema/device-apis.schema.json
           --template ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
           --output ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     DEPENDS ${DEVAPI_CODE_GEN}
             ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
             ${DEVAPI_LIB}/operations-api/tracing/device-mrt-trace.yaml
             ${DEVAPI_LIB}/schema/device-apis.schema.json
     )
   add_custom_target(${GEN_OUTPUT} ALL
             DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
  list(APPEND GEN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
endforeach()

add_custom_target(gen-device-mrt-trace
  DEPENDS ${GEN_FILES}
  )

add_library(device-mrt-trace STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/device-mrt-trace.c
  ${CMAKE_CURRENT_BINARY_DIR}/device-mrt-trace.h
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_helpers.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ring_buffer.c
  )

list(APPEND INC_PATH ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(device-mrt-trace PUBLIC
  "$<BUILD_INTERFACE:${INC_PATH}>"
  "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/esperanto-fw/device-mrt-trace>"
  )

install(TARGETS device-mrt-trace
  EXPORT  EsperantoDeviceMinionRuntimeTargets
  ARCHIVE
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw
    COMPONENT device-mrt-trace
  PUBLIC_HEADER
    DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto-fw/device-mrt-trace
    COMPONENT device-mrt-trace
  )
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/device-mrt-trace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/trace_helpers.h
  DESTINATION
    ${INCLUDE_INSTALL_DIR}/esperanto-fw/device-mrt-trace
  COMPONENT device-mrt-trace
  )
