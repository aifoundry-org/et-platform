# - Try to find libunwind
# Once done this will define
#  LIBUNWIND_FOUND - System has libunwind
#  LIBUNWIND_LIBRARIES - The libraries needed to use libunwind
#  LIBUNWIND_INCLUDE_DIRS - The libunwind include directories
#  LIBUNWIND_DEFINITIONS - Compiler switches required for using libunwind

find_path(LIBUNWIND_INCLUDE_DIR libunwind.h)
find_library(LIBUNWIND_LIBRARY libunwind.a unwind PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(libunwind DEFAULT_MSG
                                  LIBUNWIND_LIBRARY LIBUNWIND_INCLUDE_DIR)

mark_as_advanced(LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARY)

set(LIBUNWIND_LIBRARIES ${LIBUNWIND_LIBRARY})
set(LIBUNWIND_INCLUDE_DIRS ${LIBUNWIND_INCLUDE_DIR})
set(LIBUNWIND_DEFINITIONS "")
