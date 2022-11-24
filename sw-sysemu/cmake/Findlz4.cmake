# WHEN USING CONAN THIS SHOULD NOT BE USED
# CMake find_package() Module for LZ4 compression library
#
# Example usage:
#
# find_package(LZ4)
#
# If successful the following variables will be defined
# - LZ4_FOUND - System has lz4
# If successful the following targets will be defined
# - lz4
# - lz4::lz4

include(FindPackageHandleStandardArgs)

find_program(LZ4_EXECUTABLE NAMES lz4)
find_path(LZ4_INCLUDE_DIR lz4.h)
find_library(LZ4_LIBRARY NAMES liblz4.a lz4)

find_package_handle_standard_args(lz4 DEFAULT_MSG LZ4_EXECUTABLE LZ4_INCLUDE_DIR LZ4_LIBRARY)

if (LZ4_FOUND)
    mark_as_advanced(LZ4_EXECUTABLE LZ4_INCLUDE_DIR LZ4_LIBRARY)

    if (NOT TARGET lz4)
        add_library(lz4 UNKNOWN IMPORTED GLOBAL)
        target_include_directories(lz4 INTERFACE ${LZ4_INCLUDE_DIR})
        set_target_properties(lz4 PROPERTIES IMPORTED_LOCATION ${LZ4_LIBRARY})
    endif()
    if (TARGET lz4 AND NOT TARGET lz4::lz4)
        add_library(lz4::lz4 ALIAS lz4)
    endif()

endif()
