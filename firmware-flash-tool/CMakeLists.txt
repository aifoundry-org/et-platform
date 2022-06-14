#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.15)
project(EsperantoFlashTool VERSION 1.0.0 LANGUAGES C)

include(GNUInstallDirs)

### dependencies

find_package(json-c REQUIRED)

### project options
option(ENABLE_WARNINGS_AS_ERRORS "" OFF)

###

add_library(headers INTERFACE)
add_library(esperanto-flash-tool::headers ALIAS headers)
target_include_directories(headers
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto/flash-tool>
)
set_target_properties(headers PROPERTIES
    PUBLIC_HEADER "include/esperanto_flash_image.h"
)

add_executable(esperanto_flash_tool
    src/esperanto_flash_tool.c
    src/esperanto_flash_image_create.c
    src/esperanto_flash_image_extract_all.c
    src/esperanto_flash_image_extract.c
    src/esperanto_flash_image_view.c
    src/esperanto_flash_image_replace.c
    src/esperanto_flash_image_util.c
    src/esperanto_flash_tool.c
    src/parse_template_file.c
)
target_include_directories(esperanto_flash_tool
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto/flash-tool>
)
target_compile_options(esperanto_flash_tool
    PRIVATE
        -Wall -Wextra $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror> -Wnull-dereference
        -Wduplicated-branches -Wduplicated-cond -Wshadow -Wpointer-arith -Wundef
        -Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op
        -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
)
target_link_libraries(esperanto_flash_tool
    PRIVATE
        json-c::json-c
        esperanto-flash-tool::headers
)


### Installation

# install targets
install(TARGETS headers esperanto_flash_tool
    EXPORT EsperantoFlashToolTargets
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/flash-tool
)

# Install the export set for use with the install-tree
install(EXPORT EsperantoFlashToolTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EsperantoFlashTool
    NAMESPACE esperanto-flash-tool::
)

# install scripts
install(PROGRAMS scripts/make_zebu_flash_image.py
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)


# cmake package installation

include(CMakePackageConfigHelpers)


# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/EsperantoFlashToolConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoFlashToolConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EsperantoFlashTool
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoFlashToolConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoFlashToolConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoFlashToolConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EsperantoFlashTool
)

