#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

include(ExternalProject)

if (NOT __GFLAGS_INCLUDED) # guard against multiple includes
  set(__GFLAGS_INCLUDED TRUE)

  if (gflags_FOUND)
    set(GFLAGS_EXTERNAL FALSE)
    set(GFLAGS_LIBRARIES gflags ${CMAKE_THREAD_LIBS_INIT} PARENT_SCOPE)
    set(GFLAGS_LIBRARY_DIRS /lib CACHE PATH "gflags library dirs")

  else()
    # gflags will use pthreads if it's available in the system, so we must link with it
    find_package(Threads)

    # build directory
    set(gflags_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gflags-prefix)
    # install directory
    set(gflags_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})

    # we build gflags statically, but want to link it into the caffe shared library
    # this requires position-independent code
    if (UNIX)
        set(GFLAGS_EXTRA_COMPILER_FLAGS "-fPIC")
    endif()

    set(GFLAGS_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${GFLAGS_EXTRA_COMPILER_FLAGS})
    set(GFLAGS_C_FLAGS ${CMAKE_C_FLAGS} ${GFLAGS_EXTRA_COMPILER_FLAGS})

    ExternalProject_Add(gflags
      PREFIX ${gflags_PREFIX}
      GIT_REPOSITORY "git@gitlab.esperanto.ai:software/gflags-mirror.git"
      GIT_TAG "91179af8ebeb566830d1d6173d6d501ab71dfe47"
      GIT_SUBMODULES ""
      UPDATE_COMMAND ""
      INSTALL_DIR ${gflags_INSTALL}
      CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                 # -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                 -DCMAKE_INSTALL_PREFIX=${gflags_INSTALL}
                 -DBUILD_SHARED_LIBS=ON
                 -DBUILD_STATIC_LIBS=ON
                 -DBUILD_gflags_LIB=ON
                 -DBUILD_PACKAGING=OFF
                 -DBUILD_TESTING=OFF
                 -DBUILD_NC_TESTS=OFF
                 -DBUILD_CONFIG_TESTS=OFF
                 -DINSTALL_HEADERS=ON
                 -DCMAKE_C_FLAGS=${GFLAGS_C_FLAGS} -fPIC
                 -DCMAKE_CXX_FLAGS=${GFLAGS_CXX_FLAGS} -fPIC
                 -DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON
                 -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON
                 -DGFLAGS_STRIP_INTERNAL_FLAG_HELP=ON
       # Do not install the package cause it tries to write to the cmake registry
       # in the user's home directory
       BUILD_BYPRODUCTS ${gflags_INSTALL}/lib/libgflags.a
       LOG_DOWNLOAD 1
       LOG_CONFIGURE 1
#      LOG_INSTALL 1
      )

    set(GFLAGS_FOUND TRUE)
    set(GFLAGS_INCLUDE_DIRS ${gflags_INSTALL}/include CACHE PATH "gflags include dir")
    set(GFLAGS_LIBRARIES ${gflags_INSTALL}/lib/libgflags.a ${CMAKE_THREAD_LIBS_INIT} CACHE PATH "flags libraries")
    set(GFLAGS_LIBRARY_DIRS ${gflags_INSTALL}/lib CACHE PATH "gflags library dirs")
    set(GFLAGS_EXTERNAL TRUE)

    list(APPEND external_project_dependencies gflags)
  endif()

endif()

# glog depends on gflags

if (NOT __GLOG_INCLUDED)
  set(__GLOG_INCLUDED TRUE)

  if (GLOG_FOUND)
    set(GLOG_EXTERNAL FALSE)
    set(GLOG_LIBRARIES glog PARENT_SCOPE)
    set(GLOG_LIBRARY_DIRS lib CACHE PATH "glog library path")

  else()
    # fetch and build glog from github

    # build directory
    set(glog_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/glog-prefix)
    # install directory
    set(glog_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})

    # we build glog statically, but want to link it into the caffe shared library
    # this requires position-independent code
    if (UNIX)
      set(GLOG_EXTRA_COMPILER_FLAGS "-fPIC")
    endif()

    set(GLOG_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${GLOG_EXTRA_COMPILER_FLAGS})
    set(GLOG_C_FLAGS ${CMAKE_C_FLAGS} ${GLOG_EXTRA_COMPILER_FLAGS})

    # depend on gflags if we're also building it
    if (GFLAGS_EXTERNAL)
      set(GLOG_DEPENDS gflags)
    endif()

    ExternalProject_Add(glog
      DEPENDS ${GLOG_DEPENDS}
      PREFIX ${glog_PREFIX}
      GIT_REPOSITORY "git@gitlab.esperanto.ai:software/glog-mirror.git"
      GIT_TAG "v0.4.0"
      UPDATE_COMMAND ""
      INSTALL_DIR ${glog_INSTALL}
      PATCH_COMMAND autoreconf -i ${glog_PREFIX}/src/glog
      CONFIGURE_COMMAND env "CFLAGS=${GLOG_C_FLAGS}" "CXXFLAGS=${GLOG_CXX_FLAGS}" ${glog_PREFIX}/src/glog/configure --prefix=${glog_INSTALL} --enable-shared=no --enable-static=yes --with-gflags=${GFLAGS_LIBRARY_DIRS}/..
      BUILD_BYPRODUCTS ${glog_INSTALL}/lib/libglog.a
      LOG_DOWNLOAD 1
      LOG_CONFIGURE 1
      LOG_INSTALL 1
      )

    set(GLOG_FOUND TRUE)
    set(GLOG_INCLUDE_DIRS ${glog_INSTALL}/include ${GFLAGS_INCLUDE_DIRS}
      CACHE PATH "glog include dir")
    set(GLOG_LIBRARIES ${glog_INSTALL}/lib/libglog.a ${GFLAGS_LIBRARIES} CACHE PATH "glog libraries")
    set(GLOG_LIBRARY_DIRS ${glog_INSTALL}/lib CACHE PATH "glog library path")
    set(GLOG_EXTERNAL TRUE)

    list(APPEND external_project_dependencies glog)
  endif()

endif()

if(fmt_FOUND)
  message("fmt FOUND ${fmt_FOUND}")
  set(FMTLIB_INCLUDE_DIRS $<TARGET_PROPERTY:fmt::fmt,INTERFACE_INCLUDE_DIRECTORIES> CACHE PATH "fmtlib include dir")
  set(FMTLIB_LIBRARIES fmt::fmt CACHE STRING "fmtlib libraries")
  set(FMTLIB_LIBRARY_DIRS ${fmtlib_INSTALL}/lib64 CACHE PATH "fmtlib library path")

else()
  set(fmtlib_BUILD ${CMAKE_CURRENT_BINARY_DIR}/fmtlib-build)
  set(fmtlib_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})

  ExternalProject_Add(fmtlib
    PREFIX ${fmtlib_BUILD}
    GIT_REPOSITORY "git@gitlab.esperanto.ai:software/fmt-mirror.git"
    GIT_TAG "5.3.0"
    INSTALL_DIR ${fmtlib_INSTALL}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${fmtlib_INSTALL}
               -DCMAKE_CXX_FLAGS=-fPIC
               ${EXTERNAL_PROJECT_CCACHE}
    BUILD_BYPRODUCTS ${fmtlib_INSTALL}/lib64/libfmt.a
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_INSTALL 1
    )

  set(FMTLIB_INCLUDE_DIRS ${fmtlib_INSTALL}/include CACHE PATH "fmtlib include dir")
  set(FMTLIB_LIBRARIES ${fmtlib_INSTALL}/lib64/libfmt.a CACHE SRTING "fmtlib libraries")
  set(FMTLIB_LIBRARY_DIRS ${fmtlib_INSTALL}/lib64 CACHE PATH "fmtlib library path")

  list(APPEND external_project_dependencies fmtlib)
endif()

if("${runtime_DEVICE_API_PROVIDER}" STREQUAL "module")

  set(device_api_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})
  execute_process(COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/device-api)
  execute_process(COMMAND ${CMAKE_COMMAND} -H${CMAKE_CURRENT_SOURCE_DIR}/device_api
                     "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -fPIC"
                     -DFLATBUFFERS_BUILD_FLATLIB=ON
                     -DFLATBUFFERS_INSTALL=ON
                     -DCMAKE_INSTALL_PREFIX=${device_api_INSTALL}
                     -DBUILD_DEPENDENCIES=${BUILD_DEPENDENCIES}
                     ${EXTERNAL_PROJECT_CCACHE}
                     ${EXTERNAL_PROJECT_ENABLE_CCACHE}
    RESULT_VARIABLE result
    ERROR_VARIABLE err
    OUTPUT_VARIABLE out
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/device-api)
  if(result)
    message(FATAL_ERROR "CMake step for device-api failed ${result}: ${err} ${out}")
  endif()
  # Build the simulator_api target
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/device-api)
  if(result)
    message(FATAL_ERROR "Build step for device-api failed: ${result}")
  endif()
  message(STATUS "Building: simualtor-api...DONE")

  # Force the search location to be the one where we have installed the package
  find_package(esperanto-device-api CONFIG REQUIRED
    PATHS "${simulator_api_INSTALL}/lib/cmake/esperanto-device-api"
    NO_CMAKE_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_DEFAULT_PATH
    )

endif()

if("${runtime_SIMULATOR_API_PROVIDER}" STREQUAL "module")

  # Force build the Simulator-API as part of the cmake configure-step
  # we we are not consuming it form the outside. This will enable us to
  # use find_package as part of the configure step
  set(simulator_api_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})
  # Configure the simulator_api target
  execute_process(COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/simulator-api)
  execute_process(COMMAND ${CMAKE_COMMAND} -H${CMAKE_CURRENT_SOURCE_DIR}/simulator_api
                     -DCMAKE_INSTALL_PREFIX=${simulator_api_INSTALL}
                     -DBUILD_DEPENDENCIES=${BUILD_DEPENDENCIES}
                     ${EXTERNAL_PROJECT_CCACHE}
                     ${EXTERNAL_PROJECT_ENABLE_CCACHE}
    RESULT_VARIABLE result
    ERROR_VARIABLE err
    OUTPUT_VARIABLE out
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/simulator-api)
  if(result)
    message(FATAL_ERROR "CMake step for simulator-api failed ${result}: ${err} ${out}")
  endif()
  # Build the simulator_api target
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/simulator-api)
  if(result)
    message(FATAL_ERROR "Build step for simulator-api failed: ${result}")
  endif()
  message(STATUS "Building: simualtor-api...DONE")

  # Force the search location to be the one where we have installed the package
  find_package(SimulatorAPI CONFIG REQUIRED
    PATHS "${simulator_api_INSTALL}/lib/cmake/SimulatorAPI"
    NO_CMAKE_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_DEFAULT_PATH
    )
endif()

if("${runtime_SW_SYSEMU_PROVIDER}" STREQUAL "module")

  if(ENABLE_CCACHE)
    set(SYSEMU_CCACHE  CC=ccache\ ${CMAKE_C_COMPILER} CXX=ccache\ ${CMAKE_CXX_COMPILER})
  endif(ENABLE_CCACHE)

  if (BUILD_DEPENDECIES)
    set(EXTERN_SYSPATH "external_SYSPATH=${RT_EXTERNAL_INSTALL_PREFIX}")
  endif()

  ExternalProject_Add(sw-sysemu
    PREFIX sw-sysemu
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sw-sysemu
    CONFIGURE_COMMAND ""
    BUILD_COMMAND $(MAKE) -C <SOURCE_DIR>/sys_emu/
                          ${SYSEMU_CCACHE}
                          sysemu_OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/sysemu-build
                          libfpu_OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/sysemu-build-fpu
                          simapi_SYSPATH=${RT_EXTERNAL_INSTALL_PREFIX}
                          ${EXTERN_SYSPATH}
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/sys_emu
    INSTALL_COMMAND mkdir -p ${RT_EXTERNAL_INSTALL_PREFIX}/bin/
    && cp ${CMAKE_CURRENT_BINARY_DIR}/sysemu-build/sys_emu ${RT_EXTERNAL_INSTALL_PREFIX}/bin/
    # The dependency does not apply any more as the simulator-api should have been built
    # as part of the cmake configuration step
    # DEPENDS simulator-api
    BUILD_ALWAYS 1
    )

  if(ENABLE_SYSEMU_ARTIFACT)
    set(SYSEMU_PATH  ${REPOROOT}/swx/sw-artifacts/bin/sys_emu PARENT_SCOPE)
  else()
    set(SYSEMU_PATH  ${RT_EXTERNAL_INSTALL_PREFIX}/bin/sys_emu PARENT_SCOPE)
  endif(ENABLE_SYSEMU_ARTIFACT)

  list(APPEND external_project_dependencies sw-sysemu)

else()

  if(ENABLE_SYSEMU_ARTIFACT)
    # Use relative path for artifactory
    set(SYSEMU_PATH  ./swx/sw-artifacts/bin/sys_emu PARENT_SCOPE)
    message("SYSEMU_PATH iartifactory set to ${SYSEMU_PATH}")
  else()
    set(SYSEMU_PATH  ${DEPENDENCIES_PATH}/bin/sys_emu PARENT_SCOPE)
    message("SYSEMU_PATH package set to ${SYSEMU_PATH}")
  endif(ENABLE_SYSEMU_ARTIFACT)

endif()


if (NOT elfio_FOUND)
  if(ENABLE_CCACHE)
    set(ELFIO_CCACHE CC=ccache\ ${CMAKE_C_COMPILER} CXX=ccache\ ${CMAKE_CXX_COMPILER})
  endif(ENABLE_CCACHE)


  set(elfio_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/elfio)
  set(elfio_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})
  ExternalProject_Add(elfio
    PREFIX ${elfio_PREFIX}
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/elfio
    PATCH_COMMAND autoreconf -i <SOURCE_DIR>
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix ${elfio_INSTALL} ${ELFIO_CCACHE}
    BUILD_COMMAND $(MAKE) -j $(nproc)
    INSTALL_COMMAND $(MAKE) install
    BUILD_ALWAYS 1
    )

  list(APPEND external_project_dependencies elfio)

endif()

if(GTest_FOUND)
  set(external_test_dependencies "" PARENT_SCOPE)
else()
  set(GTEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/gtest-install CACHE INTERNAL "")
  ExternalProject_Add(googletest
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/googletest
    GIT_REPOSITORY git@gitlab.esperanto.ai:software/gtest-mirror.git
    GIT_TAG release-1.8.1
    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-src"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-build"
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest"
    CMAKE_ARGS = "-DCMAKE_INSTALL_PREFIX=${GTEST_DIR}"
                 ${EXTERNAL_PROJECT_ENABLE_CCACHE}
                 )

  set(external_test_dependencies googletest PARENT_SCOPE)
endif()

### For now include abseil as a subdirectory we pull the targets directly

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)

##############
##  ABSEIL  ##
##############
find_package(absl CONFIG)

if(NOT absl_FOUND)
  message(STATUS "Building: ABSEIL...")

  # Download, build, install abseil-cpp at configure time
  configure_file(
          ${CMAKE_CURRENT_SOURCE_DIR}/abseil.CMakeLists.txt
          ${CMAKE_CURRENT_BINARY_DIR}/abseil-cpp/CMakeLists.txt)

  execute_process(COMMAND ${CMAKE_COMMAND} -H. -Bproject_build -G "${CMAKE_GENERATOR}"
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/abseil-cpp)
  if(result)
    message(FATAL_ERROR "CMake step for abseil failed: ${result}")
  endif()

  execute_process(COMMAND ${CMAKE_COMMAND} --build project_build --config Release
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/abseil-cpp)
  if(result)
    message(FATAL_ERROR "Build step for abseil failed: ${result}")
  endif()
  message(STATUS "Building: ABSEIL...DONE")
endif()

set(external_project_dependencies ${external_project_dependencies}  PARENT_SCOPE)
