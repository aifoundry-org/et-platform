#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

include(GenerateProtocol)

jinja_generate_tf_protocol(TF_PROTOCOL_SRCS TF_PROTOCOL_HDRS
    TARGET tf-protocol-gen
    TF_PROTOCOL_CODEGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    JINJA_TEMPLATES
        tf_protocol.h.jinja
)

# header-only library for facilitating propagation of includes
add_library(tf-protocol INTERFACE)
target_include_directories(tf-protocol
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)
# Alias that should be consumed by all users
add_library(tf-protocol::tf-protocol ALIAS tf-protocol)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/esperanto/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/tf-protocol
    COMPONENT tf-protocol)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tf_specification.json
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    COMPONENT tf-protocol)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tf_specification.json
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto/tf-protocol
    COMPONENT tf-protocol)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tf_codegen.py
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto/tf-protocol
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    COMPONENT tf-protocol)
