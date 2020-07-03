#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/device_api.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/device_api.h)

file(GLOB_RECURSE DEVICE_API_SCHEMA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}} *.yaml)

# List of Jinja files to generate files for
SET(JINJA_FILES
  "device_api_types.h.jinja:device_api_types.h"
  "tracing_types.h.jinja:tracing_types.h"
  )

set(DEV_API_HDRS "${CMAKE_CURRENT_BINARY_DIR}/device_api.h"
                 "${CMAKE_CURRENT_SOURCE_DIR}/device_api_message_types.h")

foreach(TMPL_TUPLE ${JINJA_FILES})
  string(REPLACE ":" ";" TMPL_LIST ${TMPL_TUPLE})
  list(GET TMPL_LIST 0 TMPL)
  list(GET TMPL_LIST 1 GEN_OUTPUT)
  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/device_api_codegen.py
     ARGS  --spec ${CMAKE_CURRENT_SOURCE_DIR}/api/device-api.yaml
           --schema ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-api.schema.json
           --template ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
           --output ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/device_api_codegen.py
             ${DEVICE_API_SCHEMA_FILES}
             ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-api.schema.json
             ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
     )
   add_custom_target(${GEN_OUTPUT} ALL
             DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
  list(APPEND DEV_API_HDRS ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
endforeach()

add_custom_target(gen-device-api
  DEPENDS ${DEV_API_HDRS})


install(FILES ${DEV_API_HDRS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto/device-api
  COMPONENT device-api)


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/api
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-api/
  COMPONENT device-api)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-api.schema.json
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-api/schema
  COMPONENT device-api)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/device_api_codegen.py
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-api/
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  COMPONENT device-api)
