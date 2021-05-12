# - Try to find libunwind
# Once done this will define
#  LIBUNWIND_FOUND - System has libunwind
#  LIBUNWIND_LIBRARIES - The libraries needed to use libunwind
#  LIBUNWIND_INCLUDE_DIRS - The libunwind include directories
#  LIBUNWIND_DEFINITIONS - Compiler switches required for using libunwind

include(FindPackageHandleStandardArgs)

find_path(LIBUNWIND_INCLUDE_DIR libunwind.h)
find_library(LIBUNWIND_LIBRARY libunwind.a unwind PATH_SUFFIXES lib)

find_package_handle_standard_args(libunwind DEFAULT_MSG LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARY)
mark_as_advanced(LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARY)

if (libunwind_FOUND)
    add_library(libunwind UNKNOWN IMPORTED GLOBAL)
    add_library(libunwind::libunwind ALIAS libunwind)
    target_include_directories(libunwind INTERFACE ${LIBUNWIND_INCLUDE_DIR})
    set_target_properties(libunwind PROPERTIES IMPORTED_LOCATION ${LIBUNWIND_LIBRARY})
endif()
