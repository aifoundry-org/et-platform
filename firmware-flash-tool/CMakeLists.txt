#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required (VERSION 3.5)

project (FlashTool VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)
set(CMAKE_BUILD_TYPE Debug)

set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")

find_library(JSON_LIB json-c HINTS OPENSSL_DIR)
if(NOT JSON_LIB)
  message(FATAL_ERROR "libjson-c library not found")
endif()

file(MAKE_DIRECTORY build)

include_directories("include")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g \
-Wall -Wextra -Werror -Wnull-dereference \
-Wduplicated-branches -Wduplicated-cond -Wshadow -Wpointer-arith -Wundef \
-Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op \
-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations")

set(SOURCES
    src/esperanto_flash_tool.c
    src/esperanto_flash_image_create.c
    src/esperanto_flash_image_extract_all.c
    src/esperanto_flash_image_extract.c
    src/esperanto_flash_image_view.c
    src/esperanto_flash_image_replace.c
    src/esperanto_flash_image_util.c
    src/esperanto_flash_tool.c
    src/parse_template_file.c)

ADD_EXECUTABLE( esperanto_flash_tool ${SOURCES} )
target_link_libraries (esperanto_flash_tool json-c)
install(TARGETS esperanto_flash_tool EXPORT EsperantoFlashToolTargets)

add_library(flash-tool INTERFACE)
target_include_directories(flash-tool INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto/flash-tool>"
  )

install(
  TARGETS flash-tool
  EXPORT EsperantoFlashToolTargets
  LIBRARY
    DESTINATION lib
    COMPONENT flash-tool
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/flash-tool
    COMPONENT flash-tool
  )
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/flash-tool
  )


### Add package information
include(CMakePackageConfigHelpers)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE EsperantoFlashTool)

# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(EsperantoFlashToolConfig.cmake.in
     ${CMAKE_CURRENT_BINARY_DIR}/EsperantoFlashToolConfig.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/EsperantoFlashTool/cmake
  PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
  )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/EsperantoFlashToolConfigVersion.cmake
  VERSION ${VERSION}
  COMPATIBILITY SameMajorVersion )

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoFlashToolConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoFlashToolConfigVersion.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoFlashTool
  COMPONENT flash_tool)

# Install the export set for use with the install-tree
install(EXPORT EsperantoFlashToolTargets DESTINATION
  "${LIB_INSTALL_DIR}/cmake/EsperantoFlashTool" COMPONENT flash_tool)
