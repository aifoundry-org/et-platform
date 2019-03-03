function(PROTOBUF_GENERATE_C SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_C() called without any proto files")
    return()
  endif()

  if(PROTOBUF_GENERATE_C_APPEND_PATH)
    # Create an include path for each file specified
    foreach(FIL ${ARGN})
      get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
      get_filename_component(ABS_PATH ${ABS_FIL} PATH)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  else()
    set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if(DEFINED PROTOBUF_IMPORT_DIRS AND NOT DEFINED Protobuf_IMPORT_DIRS)
    set(Protobuf_IMPORT_DIRS "${PROTOBUF_IMPORT_DIRS}")
  endif()
 if(DEFINED PROTOBUF_IMPORT_DIRS AND NOT DEFINED Protobuf_IMPORT_DIRS)
    set(Protobuf_IMPORT_DIRS "${PROTOBUF_IMPORT_DIRS}")
  endif()

  if(DEFINED Protobuf_IMPORT_DIRS)
    foreach(DIR ${Protobuf_IMPORT_DIRS})
      get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  endif()

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    if(NOT PROTOBUF_GENERATE_CPP_APPEND_PATH)
      get_filename_component(FIL_DIR ${FIL} DIRECTORY)
      if(FIL_DIR)
        set(FIL_WE "${FIL_DIR}/${FIL_WE}")
      endif()
    endif()

      list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.c")
    list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.h")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.c"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.h"
      COMMAND  ${Protobuf_PROTOC-C_EXECUTABLE}
      ARGS --c_out  ${CMAKE_CURRENT_BINARY_DIR} ${_protobuf_include_path} ${ABS_FIL}
      DEPENDS ${ABS_FIL} ${Protobuf_PROTOC-C_EXECUTABLE}
      COMMENT "Running C protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

find_program(Protobuf_PROTOC-C_EXECUTABLE
    NAMES protoc-c
    DOC "The Google Protocol Buffers Compiler C"
    PATHS
    ${Protobuf_SRC_ROOT_FOLDER}/vsprojects/${_PROTOBUF_ARCH_DIR}Release
    ${Protobuf_SRC_ROOT_FOLDER}/vsprojects/${_PROTOBUF_ARCH_DIR}Debug
)
mark_as_advanced(Protobuf_PROTOC-C_EXECUTABLE)
