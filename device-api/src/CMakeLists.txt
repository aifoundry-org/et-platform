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
    ${CMAKE_CURRENT_BINARY_DIR}/esperanto/device-apis/device_apis.h)

set(DEV_API_HDRS ${CMAKE_CURRENT_BINARY_DIR}/device_apis.h
    ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_message_types.h)

include(GenerateApi)

jinja_generate_apis_cpp(DEVICE_API_OPS_SRCS DEVICE_API_OPS_HDRS OPERATIONS
    TARGET deviceApi-gen-ops
    DEVICE_API_CODEGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    JINJA_TEMPLATES
        device_apis_cxx.h.jinja
        device_apis_spec.h.jinja
        device_apis_rpc_types.h.jinja
        device_apis_tracing_types.h.jinja
)

jinja_generate_apis_cpp(DEVICE_API_MGMT_SRCS DEVICE_API_MGMT_HDRS MANAGEMENT
    TARGET deviceApi-gen-mgmt
    DEVICE_API_CODEGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    JINJA_TEMPLATES
        device_apis_cxx.h.jinja
        device_apis_spec.h.jinja
        device_apis_rpc_types.h.jinja
)

# header-only library for facilitating propagation of includes
add_library(deviceApi INTERFACE)
target_include_directories(deviceApi
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)
# Alias that should be consumed by all users
add_library(deviceApi::deviceApi ALIAS deviceApi)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_message_types.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/device-apis
    COMPONENT deviceApi)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/esperanto/device-apis/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/device-apis
    COMPONENT deviceApi)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/operations-api
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto/deviceApi
    COMPONENT deviceApi)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/management-api
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto/deviceApi
    COMPONENT deviceApi)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-apis.schema.json
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto/deviceApi/schema
    COMPONENT device-api)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_codegen.py
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto/deviceApi
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    COMPONENT device-api)


if (DEVICEAPI_DEPRECATED)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_message_types.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto/device-apis
        COMPONENT device-api)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/esperanto/device-apis/
        DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto/device-apis
        COMPONENT device-api)

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/operations-api
        DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-apis
        COMPONENT device-api)

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/management-api
        DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-apis
        COMPONENT device-api)

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/device-apis.schema.json
        DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-apis/schema
        COMPONENT device-api)

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_codegen.py
        DESTINATION ${LIB_INSTALL_DIR}/esperanto/device-apis
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
        COMPONENT device-api)
endif ()