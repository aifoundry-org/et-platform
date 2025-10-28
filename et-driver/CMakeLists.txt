cmake_minimum_required(VERSION 3.15)

project(linuxDriver
    VERSION 1.0.0
    DESCRIPTION "et-soc1 driver userspace headers"
    LANGUAGES NONE)

# This is a header-only package for userspace applications
# The actual kernel driver (.c files) is built separately

# List of header files to install
set(ET_DRIVER_HEADERS
    et_ioctl.h
)

# Install all headers
install(FILES ${ET_DRIVER_HEADERS}
    DESTINATION include
    COMPONENT et-driver-headers)

# Create and install package config file
set(ET_DRIVER_CONFIG_CONTENT "
# linuxDriverConfig.cmake
# Configuration file for et-driver userspace headers

set(linuxDriver_VERSION ${PROJECT_VERSION})
set(linuxDriver_FOUND TRUE)

# Include directory for et-driver headers
get_filename_component(_IMPORT_PREFIX \"\${CMAKE_CURRENT_LIST_DIR}/../../../\" ABSOLUTE)
set(linuxDriver_INCLUDE_DIR \"\${_IMPORT_PREFIX}/include\")

# Create interface target for modern CMake usage
if(NOT TARGET linuxDriver::linuxDriver)
    add_library(linuxDriver::linuxDriver INTERFACE IMPORTED)
    set_target_properties(linuxDriver::linuxDriver PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES \"\${linuxDriver_INCLUDE_DIR}\")
endif()

# Legacy variables for compatibility
set(LINUX_DRIVER_INCLUDE_DIR \${linuxDriver_INCLUDE_DIR})
set(LINUX_DRIVER_FOUND TRUE)
set(LINUX_DRIVER_VERSION \${linuxDriver_VERSION})

# For compatibility with different naming conventions
set(et-driver_FOUND TRUE)
set(et-driver_VERSION \${linuxDriver_VERSION})
set(et-driver_INCLUDE_DIR \${linuxDriver_INCLUDE_DIR})
")

# Write the config file to build directory and install it
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/linuxDriverConfig.cmake"
    "${ET_DRIVER_CONFIG_CONTENT}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/linuxDriverConfig.cmake"
    DESTINATION lib/cmake/linuxDriver
    COMPONENT et-driver-config)

# Create version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/linuxDriverConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/linuxDriverConfigVersion.cmake"
    DESTINATION lib/cmake/linuxDriver
    COMPONENT et-driver-config)

# Display summary
message(STATUS "et-driver (linuxDriver) header package configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Headers: ${ET_DRIVER_HEADERS}")
message(STATUS "  Optional headers: ${ET_DRIVER_OPTIONAL_HEADERS}")
message(STATUS "  Install destination: \${CMAKE_INSTALL_PREFIX}/include")
