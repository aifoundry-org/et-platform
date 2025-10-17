include(ExternalProject)

if(NOT PROJECT_BASE_DIR)
   set(PROJECT_BASE_DIR ".")
endif()

# Assume platform in /opt/et unless specified
if(NOT DEFINED ET_PLATFORM_PATH)
   set(ET_PLATFORM_PATH "/opt/et")
endif()

# Use platform toolchain unless specified.
if (NOT DEFINED TOOLCHAIN_DIR)
   set(TOOLCHAIN_DIR ${ET_PLATFORM_PATH})
endif()

if (NOT DEFINED STAGING_DIR)
   set(STAGING_DIR ${CMAKE_INSTALL_PREFIX})
endif()

list(APPEND CMAKE_MODULE_PATH ${ET_PLATFORM_PATH}/lib/cmake/cmake-modules)
list(APPEND CMAKE_PREFIX_PATH ${ET_PLATFORM_PATH}/lib/cmake)
list(APPEND CMAKE_PREFIX_PATH ${ET_PLATFORM_PATH}/lib/cmake/cmake-modules)

set(EXTERNAL_CACHE_FILE "${CMAKE_BINARY_DIR}/external_cache.cmake")
file(WRITE "${EXTERNAL_CACHE_FILE}"
    "set(CMAKE_MODULE_PATH \"${CMAKE_MODULE_PATH}\" CACHE PATH \"Module paths for external project\" FORCE)\n
     set(CMAKE_PREFIX_PATH \"${CMAKE_PREFIX_PATH}\" CACHE PATH \"Prefix paths for external project\" FORCE)\n"
)


function(ThirdParty name url tag cmake_args)
    if(cmake_args)
        separate_arguments(args_list UNIX_COMMAND "${cmake_args}")
    endif()

    ExternalProject_Add(
        ${name}
        GIT_REPOSITORY ${url}
        GIT_TAG ${tag}
        GIT_SHALLOW TRUE
        PREFIX ${CMAKE_BINARY_DIR}/${PROJECT_BASE_DIR}/${name}
        CMAKE_ARGS
                   -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
                   -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF
                   -DCMAKE_INSTALL_RPATH=$ORIGIN/../lib
                   -DCMAKE_BUILD_RPATH=${STAGING_DIR}/lib
                   -DCMAKE_INSTALL_LIBDIR=lib
                   ${args_list}
    )

    ExternalProject_Get_Property(${name} binary_dir)
    set(${name}_BINARY_DIR ${binary_dir} PARENT_SCOPE)
endfunction()

function(HostProject name cmake_args)
    if(cmake_args)
        separate_arguments(args_list UNIX_COMMAND "${cmake_args}")
    endif()

    ExternalProject_Add(
        ${name}
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_BASE_DIR}/${name}
        DEPENDS ${ARGN}
        CMAKE_ARGS
                   -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
                   -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF
                   -DCMAKE_INSTALL_RPATH=$ORIGIN/../lib
                   -DCMAKE_BUILD_RPATH=${STAGING_DIR}/lib
                   -DCMAKE_PREFIX_PATH=${STAGING_DIR}
                     -DCMAKE_INSTALL_LIBDIR=lib
                   "-C${EXTERNAL_CACHE_FILE}"
                   ${args_list}
    )

    ExternalProject_Get_Property(${name} binary_dir)
    set(${name}_BINARY_DIR ${binary_dir} PARENT_SCOPE)
endfunction()

function(HostProjectNoInstall name cmake_args)
    if(cmake_args)
        separate_arguments(args_list UNIX_COMMAND "${cmake_args}")
    endif()

    ExternalProject_Add(
        ${name}
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_BASE_DIR}/${name}
        DEPENDS ${ARGN}
        CMAKE_ARGS
                   -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
                   -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF
                   -DCMAKE_INSTALL_RPATH=$ORIGIN/../lib
                   -DCMAKE_BUILD_RPATH=${STAGING_DIR}/lib
                   -DCMAKE_PREFIX_PATH=${STAGING_DIR}
                       -DCMAKE_INSTALL_LIBDIR=lib
                   "-C${EXTERNAL_CACHE_FILE}"
                   ${args_list}
        INSTALL_COMMAND ""
    )

    ExternalProject_Get_Property(${name} binary_dir)
    set(${name}_BINARY_DIR ${binary_dir} PARENT_SCOPE)
endfunction()

#
# Note: CMAKE_BUILD_TYPE is set to Release for now.
#
function(DeviceProject name cmake_args)
    if(cmake_args)
        separate_arguments(args_list UNIX_COMMAND "${cmake_args}")
    endif()

    ExternalProject_Add(
        ${name}
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_BASE_DIR}/${name}
        DEPENDS ${ARGN}
        CMAKE_ARGS
                   -DCMAKE_BUILD_TYPE=Release
                   -DCMAKE_TOOLCHAIN_FILE=${ET_PLATFORM_PATH}/lib/cmake/riscv64-ec-toolchain.cmake
                   -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
                   -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF
                   -DCMAKE_INSTALL_RPATH=$ORIGIN/../lib
                   -DCMAKE_BUILD_RPATH=${STAGING_DIR}/lib
                   -DCMAKE_PREFIX_PATH=${STAGING_DIR}
                       -DCMAKE_INSTALL_LIBDIR=lib
                   "-C${EXTERNAL_CACHE_FILE}"
                   ${args_list}
    )


    ExternalProject_Get_Property(${name} binary_dir)
    set(${name}_BINARY_DIR ${binary_dir} PARENT_SCOPE)
endfunction()

#
# Note: CMAKE_BUILD_TYPE is set to Release for now.
#
function(DeviceProjectNoInstall name cmake_args)
    if(cmake_args)
        separate_arguments(args_list UNIX_COMMAND "${cmake_args}")
    endif()

    ExternalProject_Add(
        ${name}
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_BASE_DIR}/${name}
        DEPENDS ${ARGN}
        CMAKE_ARGS
                   -DCMAKE_BUILD_TYPE=Release
                   -DCMAKE_TOOLCHAIN_FILE=${ET_PLATFORM_PATH}/lib/cmake/riscv64-ec-toolchain.cmake
                   -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
                   -DCMAKE_BUILD_WITH_INSTALL_RPATH=OFF
                   -DCMAKE_INSTALL_RPATH=$ORIGIN/../lib
                   -DCMAKE_BUILD_RPATH=${STAGING_DIR}/lib
                   -DCMAKE_INSTALL_LIBDIR=lib
                   -DCMAKE_PREFIX_PATH=${STAGING_DIR}
                   "-C${EXTERNAL_CACHE_FILE}"
                   ${args_list}
        INSTALL_COMMAND ""
)

    ExternalProject_Get_Property(${name} binary_dir)
    set(${name}_BINARY_DIR ${binary_dir} PARENT_SCOPE)
endfunction()

