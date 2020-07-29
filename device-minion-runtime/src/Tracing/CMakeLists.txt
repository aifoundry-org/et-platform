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
set(DEVICE_MRT_TRACE_API_HDRS "${DEVAPI_LIB}/device-mrt-trace.h")

# List of Jinja files to generate files for
SET(JINJA_FILES
  "device-mrt-trace-gen.c.jinja:device-mrt-trace-gen.c"
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
     ARGS  --spec ${DEVAPI_LIB}/api/device-api.yaml
           --schema ${DEVAPI_LIB}/schema/device-api.schema.json
           --template ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
           --output ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     DEPENDS ${DEVAPI_CODE_GEN}
             ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
     )
   add_custom_target(${GEN_OUTPUT} ALL
             DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
  list(APPEND GEN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
endforeach()

add_custom_target(gen-device-mrt-trace-api
  DEPENDS ${DEVAPI_LIB})

add_library(device-mrt-trace STATIC
    ${GEN_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/device-mrt-trace.c
    )

install(TARGETS device-mrt-trace
        EXPORT  EsperantoDeviceMinionRuntimeTargets
        ARCHIVE
           DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw
           COMPONENT device-mrt-trace
        )
