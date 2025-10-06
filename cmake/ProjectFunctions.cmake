include(ExternalProject)

if(NOT PROJECT_BASE_DIR)
   set(PROJECT_BASE_DIR ".")
endif()

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
# Note: CMAKE_BUILD_TYPE is voluntarily left unset for now.
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
                   -DCMAKE_TOOLCHAIN_FILE=${STAGING_DIR}/lib/cmake/riscv64-ec-toolchain.cmake
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
# Note: CMAKE_BUILD_TYPE is voluntarily left unset for now.
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
                   -DCMAKE_TOOLCHAIN_FILE=${STAGING_DIR}/lib/cmake/riscv64-ec-toolchain.cmake
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

