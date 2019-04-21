#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

file(GLOB COMMAND_ETAPI "commands/*.fbs" "include/*.fbs" "responses/*.fbs" "events/*.fbs")

set(FLATBUFFERS_FLATC_SCHEMA_EXTRA_ARGS --scoped-enums --gen-mutable --gen-object-api --gen-compare)

############################### C++ library ###################################
build_et_flatbuffers(
  "${COMMAND_ETAPI}"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  flatc-fbs-parse
  ""
  ${CMAKE_CURRENT_BINARY_DIR}/flatc-gen-code/
  ${CMAKE_CURRENT_BINARY_DIR}/flatc-jsonschema/
  ${CMAKE_CURRENT_BINARY_DIR}/flatc-gen-cpp-raw/
  ${CMAKE_CURRENT_BINARY_DIR}/schemas
  ${CMAKE_CURRENT_BINARY_DIR}/text-schemas
  )

get_target_property(DEVICE_API_CXX_HDRS_DIR flatc-fbs-parse GENERATED_INCLUDES_DIR)

install(DIRECTORY ${DEVICE_API_CXX_HDRS_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/device-api-cxx)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/tools/flatbuffers/include/flatbuffers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS flatbuffers
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

################################ C library ####################################

# Making a submodule  flatcc has proven to be pain just clone from source
set(FLATCC_VERSION aa5bf0e2592ec251b0b684ecfa5ab066f090f283)
set(FLATCC_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/flatcc-build)
set(FLATCC_BUILD_DONE ${CMAKE_CURRENT_BINARY_DIR}/flatcc-build.done)
set(FLATCC ${CMAKE_CURRENT_BINARY_DIR}/flatcc-build/bin/flatcc)

add_custom_command(
  OUTPUT ${FLATCC_BUILD_DONE} ${FLATCC}
  COMMAND rm -rf ${FLATCC_FOLDER}
  && git clone git@gitlab:software/flatcc-mirror.git ${FLATCC_FOLDER}
  && cd ${FLATCC_FOLDER}
  && git checkout -f ${FLATCC_VERSION}
  && scripts/initbuild.sh make # there is no ninja build tool
  && scripts/build.sh
  && date > ${FLATCC_BUILD_DONE}
  VERBATIM)

install(DIRECTORY ${FLATCC_FOLDER}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${FLATCC_FOLDER}/lib/
  DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(FLATCC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/flatcc-output)
file(MAKE_DIRECTORY ${FLATCC_OUTPUT})

foreach(ETAPI_FILE ${COMMAND_ETAPI})
  get_filename_component(ETAPI_BASENAME ${ETAPI_FILE} NAME_WE)
  set(ETAPI_C_HDR "${FLATCC_OUTPUT}/${ETAPI_BASENAME}_reader.h"
                  "${FLATCC_OUTPUT}/${ETAPI_BASENAME}_verifier.h"
                  "${FLATCC_OUTPUT}/${ETAPI_BASENAME}_builder.h")
  list(APPEND ETAPI_C_PARSED ${ETAPI_C_HDR})

  add_custom_command(
    OUTPUT ${ETAPI_C_HDR}
    COMMAND ${FLATCC} ${ETAPI_FILE} -I${CMAKE_CURRENT_SOURCE_DIR}/include -a -o ${FLATCC_OUTPUT}
    MAIN_DEPENDENCY ${ETAPI_FILE}
    DEPENDS ${FLATCC_BUILD_DONE} ${FLATCC}
    VERBATIM)

  install(FILES ${ETAPI_C_HDR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/device-api-c)

endforeach()

add_custom_target(parse-device-api ALL
  DEPENDS flatc-fbs-parse ${ETAPI_C_PARSED}
  VERBATIM)

add_subdirectory(tests)
