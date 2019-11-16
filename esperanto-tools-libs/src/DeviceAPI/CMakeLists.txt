#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(DEVAPI_LIB ${ESPERANTO_DEVICE_API_LIB_DIR}/esperanto/device-api)
set(DEVAPI_CODE_GEN ${DEVAPI_LIB}/device_api_codegen.py)

# List of Jinja files to generate files for
SET(JINJA_FILES
  "CommandsGen.h.jinja:CommandsGen.h"
  "CommandsGen.cc.jinja:CommandsGen.cc"
  "EventsGen.h.jinja:EventsGen.h"
  "EventsGen.cc.jinja:EventsGen.cc"
  "ResponsesGen.h.jinja:ResponsesGen.h"
  "ResponsesGen.cc.jinja:ResponsesGen.cc"
  )

set(GEN_FILES "")

foreach(TMPL_TUPLE ${JINJA_FILES})
  string(REPLACE ":" ";" TMPL_LIST ${TMPL_TUPLE})
  list(GET TMPL_LIST 0 TMPL)
  list(GET TMPL_LIST 1 GEN_OUTPUT)
  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     COMMAND ${DEVAPI_CODE_GEN}
     ARGS  --spec ${DEVAPI_LIB}/schema/device-api.yaml
           --schema ${DEVAPI_LIB}/schema/device-api.schema.json
           --template ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
           --output ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     DEPENDS ${DEVAPI_CODE_GEN}
             ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
     )
   add_custom_target(${GEN_OUTPUT}
             DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
  list(APPEND GEN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
endforeach()

add_custom_target(etrt-device-api-gen-code
  DEPENDS ${GEN_FILES}
  )


add_library(etrt_DeviceAPI OBJECT
  Commands.cc
  Commands.h
  ${CMAKE_CURRENT_BINARY_DIR}/CommandsGen.cc
  ${CMAKE_CURRENT_BINARY_DIR}/CommandsGen.h
  ${CMAKE_CURRENT_BINARY_DIR}/EventsGen.cc
  ${CMAKE_CURRENT_BINARY_DIR}/EventsGen.h
  ${CMAKE_CURRENT_BINARY_DIR}/ResponsesGen.cc
  ${CMAKE_CURRENT_BINARY_DIR}/ResponsesGen.h
  )

add_dependencies(etrt_DeviceAPI
  etrt-device-api-gen-code
  )

target_include_directories(etrt_DeviceAPI PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/src
  ${EXTERNAL_INCLUDE_DIRECTORIES}
  )

if(esperanto-fw_FOUND)
  target_include_directories(etrt_DeviceAPI PRIVATE "${ESPERANTO_FW_INCLUDE_DIR}/")
endif(esperanto-fw_FOUND)
