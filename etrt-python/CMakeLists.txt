cmake_minimum_required(VERSION 3.15...3.27)
project(etrt_python VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure RTTI is enabled (required for nanobind and C++ exceptions)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")

# Configure ET platform path
if(DEFINED ENV{ET_PLATFORM})
    set(ET_PLATFORM_PATH $ENV{ET_PLATFORM})
else()
    set(ET_PLATFORM_PATH "/opt/et")
endif()

# Add ET platform CMake modules and config files to search paths
list(APPEND CMAKE_PREFIX_PATH ${ET_PLATFORM_PATH}/lib/cmake)
list(APPEND CMAKE_MODULE_PATH ${ET_PLATFORM_PATH}/lib/cmake)

message(STATUS "Using ET Platform at ${ET_PLATFORM_PATH}")

# Find dependencies
find_package(runtime REQUIRED)
find_package(deviceLayer REQUIRED)
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Fetch nanobind during build if not already available
include(FetchContent)
if(NOT TARGET nanobind::nanobind)
  message(STATUS "Fetching nanobind from GitHub...")
  FetchContent_Declare(
    nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind.git
    GIT_TAG v2.4.0
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(nanobind)
  message(STATUS "nanobind fetched successfully")
endif()

# Create the Python extension module
nanobind_add_module(
  _etrt_native
  NB_STATIC  # Prefer static linkage
  src/bindings.cpp
  src/device_layer_bindings.cpp
  src/types_bindings.cpp
  src/runtime_bindings.cpp
  src/callbacks.cpp
)

# Link against etrt static library and deviceLayer
# Following llama.cpp pattern: use static linking to avoid ODR issues
target_link_libraries(_etrt_native PRIVATE
  runtime::etrt_static
  deviceLayer::deviceLayer
)

# Set output directory and RPATH for runtime dependencies
set_target_properties(_etrt_native PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/etrt"
  INSTALL_RPATH "${ET_PLATFORM_PATH}/lib"
  BUILD_RPATH "${ET_PLATFORM_PATH}/lib"
  BUILD_WITH_INSTALL_RPATH TRUE
)

# Determine Python site-packages directory
execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "import site; print(site.getsitepackages()[0])"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE Python_SITEARCH
)

message(STATUS "Installing to Python site-packages: ${Python_SITEARCH}")

# Installation rules
# Install the native module to site-packages
install(TARGETS _etrt_native
  LIBRARY DESTINATION "${Python_SITEARCH}/etrt"
)

# Install Python package files to site-packages
install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/etrt/"
  DESTINATION "${Python_SITEARCH}/etrt"
  FILES_MATCHING
  PATTERN "*.py"
  PATTERN "py.typed"
  PATTERN "__pycache__" EXCLUDE
  PATTERN "*.so" EXCLUDE
)
