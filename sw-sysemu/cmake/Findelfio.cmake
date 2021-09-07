# - Try to find enfio
#
# The following variables are optionally searched for defaults
#  ELFIO_ROOT_DIR:            Base directory where all ELFIO components are found
#
# The following targets are set after configuration is done:
#  elfio
#  elfio::elfio

include(FindPackageHandleStandardArgs)

set(ELFIO_ROOT_DIR "" CACHE PATH "Folder containing elfio")


find_path(ELFIO_INCLUDE_DIR NAMES elfio/elfio.hpp PATHS ${ELFIO_ROOT_DIR})

find_package_handle_standard_args(elfio DEFAULT_MSG ELFIO_INCLUDE_DIR)
mark_as_advanced(ELFIO_ROOT_DIR ELFIO_INCLUDE_DIR)

if(ELFIO_FOUND)
    if (NOT TARGET elfio)
        add_library(elfio INTERFACE IMPORTED GLOBAL)
        target_include_directories(elfio INTERFACE ${ELFIO_INCLUDE_DIR})
    endif()
    if (TARGET elfio AND NOT TARGET elfio::elfio)
        add_library(elfio::elfio ALIAS elfio)
    endif()
endif()