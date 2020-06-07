#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

set(TRACING_PB ${CMAKE_CURRENT_BINARY_DIR}/etrt-trace)

file(GLOB_RECURSE SCHEMA_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}} Schema/*.yaml)
list(APPEND SCHEMA_SRC_FILES  Schema/tracing-spec.schema.json)

set(SCHEMA_FILES "")

foreach(SCHEMA_FILE ${SCHEMA_SRC_FILES})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_FILE}
                 ${CMAKE_CURRENT_BINARY_DIR}/${SCHEMA_FILE})
  list(APPEND SCHEMA_FILES ${CMAKE_CURRENT_BINARY_DIR}/${SCHEMA_FILE})
endforeach()

# List of Jinja files to generate files for
SET(JINJA_FILES
  "etrt-trace.pb.jinja:etrt-trace"
  "TracingCommonGen.h.jinja:TracingCommonGen.h"
  "TracingGen.h.jinja:TracingGen.h"
  "TracingGen.cc.jinja:TracingGen.cc"
  )

set(GEN_FILES "")

foreach(TMPL_TUPLE ${JINJA_FILES})
  string(REPLACE ":" ";" TMPL_LIST ${TMPL_TUPLE})
  list(GET TMPL_LIST 0 TMPL)
  list(GET TMPL_LIST 1 GEN_OUTPUT)
  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tracing_codegen.py
     ARGS  --spec ${CMAKE_CURRENT_BINARY_DIR}/Schema/etrt-trace.yaml
           --schema ${CMAKE_CURRENT_BINARY_DIR}/Schema/tracing-spec.schema.json
           --template ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
           --output ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT}
     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tracing_codegen.py
             ${SCHEMA_FILES}
             ${CMAKE_CURRENT_SOURCE_DIR}/${TMPL}
     )
   add_custom_target(${GEN_OUTPUT}
             DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
  list(APPEND GEN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${GEN_OUTPUT})
endforeach()

find_package(Protobuf CONFIG REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

set(ETRT_TRACING_PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/etrt-trace.pb.cc")
set(ETRT_TRACING_PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/etrt-trace.pb.h")
set(ETRT_TRACING_PROTO_PY "${CMAKE_CURRENT_BINARY_DIR}/etrt_trace_pb2.py")

add_custom_command(
  OUTPUT ${ETRT_TRACING_PROTO_SRCS} ${ETRT_TRACING_PROTO_HDRS}
  COMMAND ${_PROTOBUF_PROTOC}
  ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/"
       -I "${CMAKE_CURRENT_BINARY_DIR}"
       "${TRACING_PB}"
  DEPENDS ${TRACING_PB}
  )

add_custom_command(
  OUTPUT  ${ETRT_TRACING_PROTO_PY}
  COMMAND python3 -m grpc_tools.protoc
  ARGS    -I"${CMAKE_CURRENT_BINARY_DIR}"
          --python_out=${CMAKE_CURRENT_BINARY_DIR}
          ${TRACING_PB}
  DEPENDS ${TRACING_PB}
  )

add_custom_target(etrt-tracing-gen-code
  DEPENDS ${GEN_FILES} ${ETRT_TRACING_PROTO_SRCS} ${ETRT_TRACING_PROTO_HDRS} ${ETRT_TRACING_PROTO_PY}
  )

add_library(etrt_Tracing OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/Tracing.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/TracingHelpers.cc
  ${ETRT_TRACING_PROTO_SRCS}
  ${ETRT_TRACING_PROTO_HDRS}
  ${CMAKE_CURRENT_BINARY_DIR}/TracingGen.cc
  ${CMAKE_CURRENT_BINARY_DIR}/TracingGen.h
  ${CMAKE_CURRENT_BINARY_DIR}/TracingCommonGen.h
  )

target_include_directories(etrt_Tracing PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/src
 )

target_link_libraries(etrt_Tracing
  protobuf)

add_sanitizers(etrt_Tracing)
