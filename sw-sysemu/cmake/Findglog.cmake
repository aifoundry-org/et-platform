# - Try to find Glog
#
# The following variables are optionally searched for defaults
#  GLOG_ROOT_DIR:            Base directory where all GLOG components are found
#
# The following targets are available after glog is found:
#  glog
#  glog::glog

include(FindPackageHandleStandardArgs)

set(GLOG_ROOT_DIR "" CACHE PATH "Folder contains Google glog")

if(WIN32)
    find_path(GLOG_INCLUDE_DIR glog/logging.h
        PATHS ${GLOG_ROOT_DIR}/src/windows)
else()
    find_path(GLOG_INCLUDE_DIR glog/logging.h
        PATHS ${GLOG_ROOT_DIR})
endif()

if(MSVC)
    find_library(GLOG_LIBRARY_RELEASE libglog_static
        PATHS ${GLOG_ROOT_DIR}
        PATH_SUFFIXES Release)

    find_library(GLOG_LIBRARY_DEBUG libglog_static
        PATHS ${GLOG_ROOT_DIR}
        PATH_SUFFIXES Debug)

    set(GLOG_LIBRARY optimized ${GLOG_LIBRARY_RELEASE} debug ${GLOG_LIBRARY_DEBUG})
else()
    find_library(GLOG_LIBRARY glog
        PATHS ${GLOG_ROOT_DIR}
        PATH_SUFFIXES lib lib64)
endif()

find_package_handle_standard_args(glog DEFAULT_MSG GLOG_INCLUDE_DIR GLOG_LIBRARY)

if (GLOG_FOUND)
    add_library(glog UNKNOWN IMPORTED GLOBAL)
    add_library(glog::glog ALIAS glog)
    target_include_directories(glog INTERFACE ${GLOG_INCLUDE_DIR})
    set_target_properties(glog PROPERTIES IMPORTED_LOCATION ${GLOG_LIBRARY})

    set(GLOG_INCLUDE_DIRS ${GLOG_INCLUDE_DIR})
    set(GLOG_LIBRARIES ${GLOG_LIBRARY})
    message(STATUS "Found glog    (include: ${GLOG_INCLUDE_DIR}, library: ${GLOG_LIBRARY})")
    mark_as_advanced(GLOG_ROOT_DIR GLOG_LIBRARY_RELEASE GLOG_LIBRARY_DEBUG
                                   GLOG_LIBRARY GLOG_INCLUDE_DIR)
endif()