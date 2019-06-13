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

  # FIXME use the system-wide gflags if present
  # find_package(GFlags)
  if (GFLAGS_FOUND)
    set(GFLAGS_EXTERNAL FALSE)
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
      GIT_REPOSITORY "https://github.com/gflags/gflags.git"
      GIT_TAG "v2.2.2"
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

  # FIXME try the system-wide glog first
  # find_package(Glog)
  if (GLOG_FOUND)
      set(GLOG_EXTERNAL FALSE)
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
      GIT_REPOSITORY "https://github.com/google/glog"
      GIT_TAG "v0.4.0"
      UPDATE_COMMAND ""
      INSTALL_DIR ${gflags_INSTALL}
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
    set(GLOG_LIBRARIES ${GFLAGS_LIBRARIES} ${glog_INSTALL}/lib/libglog.a CACHE PATH "glog libraries")
    set(GLOG_LIBRARY_DIRS ${glog_INSTALL}/lib CACHE PATH "glog library path")
    set(GLOG_EXTERNAL TRUE)

    list(APPEND external_project_dependencies glog)
  endif()

endif()


set(fmtlib_BUILD ${CMAKE_CURRENT_BINARY_DIR}/fmtlib-build)
set(fmtlib_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})

ExternalProject_Add(fmtlib
  PREFIX ${fmtlib_BUILD}
  GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
  GIT_TAG "5.3.0"
  INSTALL_DIR ${fmtlib_INSTALL}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${fmtlib_INSTALL}
             -DCMAKE_CXX_FLAGS=-fPIC
  BUILD_BYPRODUCTS ${fmtlib_INSTALL}/lib64/libfmt.a
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_INSTALL 1
  )

set(FMTLIB_INCLUDE_DIRS ${fmtlib_INSTALL}/include CACHE PATH "fmtlib include dir")
set(FMTLIB_LIBRARIES ${fmtlib_INSTALL}/lib64/libfmt.a CACHE PATH "fmtlib libraries")
set(FMTLIB_LIBRARY_DIRS ${fmtlib_INSTALL}/lib64 CACHE PATH "fmtlib library path")

list(APPEND external_project_dependencies fmtlib)

if("${runtime_DEVICE_API_PROVIDER}" STREQUAL "module")

set(device_api_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})
ExternalProject_Add(device-api
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/device_api
  INSTALL_DIR ${device_api_INSTALL}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${device_api_INSTALL}
             "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -fPIC"
             -DFLATBUFFERS_BUILD_FLATLIB=ON
             -DFLATBUFFERS_INSTALL=ON
#  BUILD_BYPRODUCTS ${device_api_INSTALL}/lib/libfmt.a
  )

list(APPEND external_project_dependencies device-api)

endif()

if("${runtime_SIMULATOR_API_PROVIDER}" STREQUAL "module")

set(simulator_api_INSTALL ${RT_EXTERNAL_INSTALL_PREFIX})
ExternalProject_Add(simulator-api
  PREFIX simulator-api
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/simulator_api
  INSTALL_DIR ${simulator_api_INSTALL}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${simulator_api_INSTALL}
  #  BUILD_BYPRODUCTS ${device_api_INSTALL}/lib/libfmt.a
  BUILD_ALWAYS 1
  )

list(APPEND external_project_dependencies simulator-api)

endif()

if("${runtime_SW_SYSEMU_PROVIDER}" STREQUAL "module")

ExternalProject_Add(sw-sysemu
  PREFIX sw-sysemu
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sw-sysemu
  CONFIGURE_COMMAND ""
  BUILD_COMMAND $(MAKE) -C <SOURCE_DIR>/sys_emu/
                    sysemu_OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/sysemu-build
                    libfpu_OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/sysemu-build-fpu
                    external_SYSPATH=${RT_EXTERNAL_INSTALL_PREFIX}
  BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/sys_emu
  INSTALL_COMMAND mkdir -p ${RT_EXTERNAL_INSTALL_PREFIX}/bin/
                    && cp ${CMAKE_CURRENT_BINARY_DIR}/sysemu-build/sys_emu ${RT_EXTERNAL_INSTALL_PREFIX}/bin/
  DEPENDS simulator-api
  BUILD_ALWAYS 1
  )

set(SYSEMU_PATH  ${RT_EXTERNAL_INSTALL_PREFIX}/bin/sys_emu PARENT_SCOPE)

list(APPEND external_project_dependencies sw-sysemu)

else()

set(SYSEMU_PATH  ${DEPENDENCIES_PATH}/bin/sys_emu PARENT_SCOPE)

endif(BUILD_SW_SYSEMU)


set(GTEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/gtest-install CACHE INTERNAL "")
ExternalProject_Add(googletest
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/googletest
  GIT_REPOSITORY git@gitlab.esperanto.ai:software/gtest-mirror.git
  GIT_TAG release-1.8.1
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-src"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-build"
  INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest"
  CMAKE_ARGS = "-DCMAKE_INSTALL_PREFIX=${GTEST_DIR}"
  )

set(external_project_dependencies ${external_project_dependencies}  PARENT_SCOPE)
