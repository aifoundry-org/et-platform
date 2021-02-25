# CMake find_package() Module for cereal library
#
# Example usage:
#
# find_package(cereal)
#
# If successful the following variables will be defined
# cereal_FOUND - System has cereal
# cereal_INCLUDE_DIRS - The cereal include directories
# cereal_LIBRARIES - The libraries needed to use cereal
# cereal_DEFINITIONS - Compiler switches required for using cereal

set(cereal_DEFINITIONS "")

find_path(cereal_INCLUDE_DIR cereal/cereal.hpp
          HINTS "${PROJECT_SOURCE_DIR}/external/cereal/include")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cereal DEFAULT_MSG cereal_INCLUDE_DIR)
mark_as_advanced(cereal_INCLUDE_DIR)

set(cereal_INCLUDE_DIRS ${cereal_INCLUDE_DIR})

if (cereal_FOUND)
  add_library(cereal INTERFACE IMPORTED GLOBAL)
  add_library(cereal::cereal ALIAS cereal)
  target_include_directories(cereal INTERFACE ${cereal_INCLUDE_DIRS})
endif()
