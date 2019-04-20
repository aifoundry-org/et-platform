#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------
include(FindProtobuf)
find_package(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIR})

protobuf_generate_cpp(SIMULATOR_API_CXX_PROTO_SRC SIMULATOR_API_CXX_PROTO_HEADER simulator-api.proto)
add_library(simulator_api_cxx SHARED ${SIIMULATOR_API_CXX_PROTO_SRC} ${SIMULATOR_API_CXX_PROTO_HEADER})
set_target_properties(simulator_api_cxx PROPERTIES LINKER_LANGUAGE CXX)

protobuf_generate_c(SIMULATOR_API_C_PROTO_SRC SIMULATOR_API_C_PROTO_HEADER simulator-api.proto)
add_library(simulator_api_c SHARED ${SIIMULATOR_API_C_PROTO_SRC} ${SIMULATOR_API_C_PROTO_HEADER})
set_target_properties(simulator_api_c PROPERTIES LINKER_LANGUAGE C)

protobuf_generate_python(SIMULATOR_API_PY simulator-api.proto)


add_custom_target(simulator-api
  DEPENDS simulator_api_cxx simulator_api_c
  ${SIMULATOR_API_C_PROTO_SRC} ${SIMULATOR_API_C_PROTO_HEADER}
  ${SIMULATOR_API_PY})
