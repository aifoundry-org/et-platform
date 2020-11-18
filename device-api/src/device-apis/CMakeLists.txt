#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/device_apis.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/device_apis.h)

file(GLOB_RECURSE DEVICE_API_SCHEMA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}} *.yaml)

set(DEV_API_HDRS "${CMAKE_CURRENT_BINARY_DIR}/device_apis.h"
                 "${CMAKE_CURRENT_SOURCE_DIR}/device_apis_message_types.h")

SET(JINJA_FILES "")

################### Device Ops API Headers #########################
# List of Jinja files to generate files for
SET(JINJA_FILES
  "device_apis_spec.h.jinja:device_ops_api_spec.h"
  "device_apis_rpc_types.h.jinja:device_ops_api_rpc_types.h"
  "device_apis_cxx.h.jinja:device_ops_api_cxx.h"
  "device_apis_tracing_types.h.jinja:device_ops_api_tracing_types.h"
  )

foreach(TMPL_TUPLE ${JINJA_FILES})
  string(REPLACE ":" ";" TMPL_LIST ${TMPL_TUPLE})
  list(GET TMPL_LIST 0 TMPL)
  list(GET TMPL_LIST 1 GEN_OUTPUT)

  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/operations-api/${GEN_OUTPUT}
     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/operations-api
     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_codegen.py
     ARGS  --spec ${CMAKE_CURRENT_SOURCE_DIR}/operations-api/operations-api.yaml
           --schema ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-apis.schema.json
           --template ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
           --output ${CMAKE_CURRENT_BINARY_DIR}/operations-api/${GEN_OUTPUT}
     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_codegen.py
             ${DEVICE_API_SCHEMA_FILES}
             ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-apis.schema.json
             ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
     )
   add_custom_target(${GEN_OUTPUT} ALL
             DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/operations-api/${GEN_OUTPUT})
  list(APPEND DEV_OPS_API_HDRS ${CMAKE_CURRENT_BINARY_DIR}/operations-api/${GEN_OUTPUT})
endforeach()

SET(JINJA_FILES "")

################### Device Management API Headers #########################
# List of Jinja files to generate files for
SET(JINJA_FILES
  "device_apis_spec.h.jinja:device_mgmt_api_spec.h"
  "device_apis_rpc_types.h.jinja:device_mgmt_api_rpc_types.h"
  "device_apis_cxx.h.jinja:device_mgmt_api_cxx.h"
  )

foreach(TMPL_TUPLE ${JINJA_FILES})
  string(REPLACE ":" ";" TMPL_LIST ${TMPL_TUPLE})
  list(GET TMPL_LIST 0 TMPL)
  list(GET TMPL_LIST 1 GEN_OUTPUT)

  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/management-api/${GEN_OUTPUT}
     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/management-api
     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_codegen.py
     ARGS  --spec ${CMAKE_CURRENT_SOURCE_DIR}/management-api/management-api.yaml
           --schema ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-apis.schema.json
           --template ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
           --output ${CMAKE_CURRENT_BINARY_DIR}/management-api/${GEN_OUTPUT}
     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_codegen.py
             ${DEVICE_API_SCHEMA_FILES}
             ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-apis.schema.json
             ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
     )
   add_custom_target(${GEN_OUTPUT} ALL
             DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/management-api/${GEN_OUTPUT})
  list(APPEND DEV_MGMT_API_HDRS ${CMAKE_CURRENT_BINARY_DIR}/management-api/${GEN_OUTPUT})
endforeach()

add_custom_target(gen-device-apis
  DEPENDS ${DEV_API_HDRS} ${DEV_OPS_API_HDRS} ${DEV_MGMT_API_HDRS})

add_custom_target(gen-device-ops-api
  DEPENDS ${DEV_API_HDRS} ${DEV_OPS_API_HDRS})

add_custom_target(gen-device-mgmt-api
  DEPENDS ${DEV_API_HDRS} ${DEV_MGMT_API_HDRS})

install(FILES ${DEV_API_HDRS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto/device-apis
  COMPONENT device-api)

install(FILES ${DEV_OPS_API_HDRS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto/device-apis/operations-api
  COMPONENT device-api)

install(FILES ${DEV_MGMT_API_HDRS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto/device-apis/management-api
  COMPONENT device-api)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/operations-api
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-apis/
  COMPONENT device-api)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/management-api
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-apis/
  COMPONENT device-api)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-apis.schema.json
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-apis/schema
  COMPONENT device-api)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_codegen.py
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-apis/
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  COMPONENT device-api)
