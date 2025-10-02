# - Try to find enfio
#
# The following variables are optionally searched for defaults
#  ELFIO_ROOT_DIR:            Base directory where all ELFIO components are found
#
# The following are set after configuration is done:
#  ELFIO_FOUND
#  ELFIO_INCLUDE_DIRS
#  ELFIO_LIBRARIES
#  ELFIO_LIBRARYRARY_DIRS

include(FindPackageHandleStandardArgs)

set(ELFIO_ROOT_DIR "" CACHE PATH "Folder contains Google glog")


find_path(ELFIO_INCLUDE_DIR
  NAMES elfio/elfio.hpp
  PATHS ${ELFIO_ROOT_DIR})

find_package_handle_standard_args(elfio DEFAULT_MSG ELFIO_INCLUDE_DIR)

if(ELFIO_FOUND)
  set(ELFIO_INCLUDE_DIRS ${ELFIO_INCLUDE_DIR})
  set(ELFIO_LIBRARIES ${ELFIO_LIBRARY})
  message(STATUS "Found elfio (include: ${ELFIO_INCLUDE_DIR}, library: ${ELFIO_LIBRARY})")
  mark_as_advanced(ELFIO_ROOT_DIR ELFIO_LIBRARY_RELEASE ELFIO_LIBRARY_DEBUG
                                 ELFIO_LIBRARY ELFIO_INCLUDE_DIR)
endif()
