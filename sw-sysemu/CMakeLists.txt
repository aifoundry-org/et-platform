#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)

project(sw-sysemu)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(SimulatorAPI CONFIG REQUIRED)
find_package(libunwind)

set(SYSEMU_LIBS_FLAGS "")

if(LIBUNWIND_FOUND)
    list(APPEND SYSEMU_LIBS_FLAGS "BACKTRACE=1")
endif()

set(LIBSYSEMU_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/libsysemu-build)
set(LIBFPU_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/libfpu-build)

# libsysemu
add_custom_target(build-sysemu-libsysemu ALL
    COMMAND ${CMAKE_MAKE_PROGRAM} ${LIBSYSEMU_BUILD_DIR}/libsysemu.a sysemu_OBJDIR=${LIBSYSEMU_BUILD_DIR} ${SYSEMU_LIBS_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sys_emu
)

add_library(libsysemu STATIC IMPORTED)
add_dependencies(libsysemu build-sysemu-libsysemu)
set_property(TARGET libsysemu
    APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG
)
set_target_properties(libsysemu
    PROPERTIES IMPORTED_LOCATION_NOCONFIG ${LIBSYSEMU_BUILD_DIR}/libsysemu.a
)

# libfpu
add_custom_target(build-sysemu-libfpu ALL
    COMMAND ${CMAKE_MAKE_PROGRAM} ${LIBFPU_BUILD_DIR}/libfpu.a libfpu_OBJDIR=${LIBFPU_BUILD_DIR} ${SYSEMU_LIBS_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sys_emu
)

add_library(libfpu STATIC IMPORTED)
add_dependencies(libfpu build-sysemu-libfpu)
set_property(TARGET libfpu
    APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG
)
set_target_properties(libfpu
    PROPERTIES IMPORTED_LOCATION_NOCONFIG ${LIBFPU_BUILD_DIR}/libfpu.a
)

# sys_emu
add_executable(sys_emu
    sw-sysemu/main.cpp
    sw-sysemu/sim_api_communicate.cpp
)

add_definitions(-DSYS_EMU)

include_directories(
    ${CMAKE_SOURCE_DIR}
    sw-sysemu/
    sys_emu/
    sys_emu/checkers/
    # TODO: Have etsim_api server-side header-only lib
    ${SIMULATOR_API_INCLUDE_DIR}
)

target_link_libraries(sys_emu
    libsysemu
    libfpu
    etsim_api_static
)

if(LIBUNWIND_FOUND)
    include_directories(
        ${LIBUNWIND_INCLUDE_DIRS}
    )

    target_link_libraries(sys_emu
        ${LIBUNWIND_LIBRARIES}
    )
endif()

install(TARGETS sys_emu
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    COMPONENT tools
)
