# - Try to find enfio
#
# The following variables are optionally searched for defaults
#  BREAKPAD_ROOT_DIR:            Base directory where all BREAKPAD components are found
#
# The following are set after configuration is done:
#  BREAKPAD_FOUND
#  BREAKPAD_INCLUDE_DIRS
#  BREAKPAD_LIBRARIES
#  BREAKPAD_LIBRARYRARY_DIRS

include(FindPackageHandleStandardArgs)

set(BREAKPAD_ROOT_DIR "" CACHE PATH "Folder contains Google breakpad")


find_path(BREAKPAD_INCLUDE_DIR
  NAMES breakpad
  PATHS ${BREAKPAD_ROOT_DIR})

find_package_handle_standard_args(breakpad DEFAULT_MSG BREAKPAD_INCLUDE_DIR)

if(BREAKPAD_FOUND)
  set(BREAKPAD_INCLUDE_DIRS ${BREAKPAD_INCLUDE_DIR})
  set(BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARY})
  message(STATUS "Found break; (include: ${BREAKPAD_INCLUDE_DIR}, library: ${BREAKPAD_LIBRARY})")
  mark_as_advanced(BREAKPAD_ROOT_DIR BREAKPAD_LIBRARY_RELEASE BREAKPAD_LIBRARY_DEBUG
                                 BREAKPAD_LIBRARY BREAKPAD_INCLUDE_DIR)
endif()
