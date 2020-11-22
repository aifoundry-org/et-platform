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
               ${CMAKE_CURRENT_BINARY_DIR}/esperanto/device-api/device_api.h)

set(DEV_API_HDRS ${CMAKE_CURRENT_BINARY_DIR}/device_api.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/device_api_message_types.h)

include(GenerateApi)

jinja_generate_api_cpp(DEVICE_API_PRIVILEGED_SRCS DEVICE_API_PRIVILEGED_HDRS PRIVILEGED
  TARGET device-api-gen-privileged
  DEVICE_API_CODEGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  JINJA_TEMPLATES
    device_api_spec.h.jinja
    device_api_rpc_types.h.jinja
    device_api_cxx.h.jinja
)

jinja_generate_api_cpp(DEVICE_API_NON_PRIVILEGED_SRCS DEVICE_API_NON_PRIVILEGED_HDRS NON_PRIVILEGED
  TARGET device-api-gen-non-privileged
  DEVICE_API_CODEGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  JINJA_TEMPLATES
    device_api_spec.h.jinja
    device_api_rpc_types.h.jinja
    device_api_cxx.h.jinja
    tracing_types.h.jinja
)

add_custom_target(device-api-gen-devices-apis
  DEPENDS ${DEV_API_HDRS} ${DEVICE_API_PRIVILEGED_HDRS} ${DEVICE_API_NON_PRIVILEGED_HDRS})

# header-only library for facilitating propagation of includes
add_library(device-api INTERFACE)
target_include_directories(device-api
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)
# Alias that should be consumed by all users
add_library(device-api::device-api ALIAS device-api)

install(TARGETS device-api
  EXPORT esperanto-device-api
  DESTINATION ${LIB_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/device_api_message_types.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto/device-api
  COMPONENT device-api)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/esperanto/device-api/
  DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto/device-api
  COMPONENT device-api)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/privileged-api
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-api
  COMPONENT device-api)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/non-privileged-api
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-api
  COMPONENT device-api)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-api.schema.json
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-api/schema
  COMPONENT device-api)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/device_api_codegen.py
  DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-api
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  COMPONENT device-api)
