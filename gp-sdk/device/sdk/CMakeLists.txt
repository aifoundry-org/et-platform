#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------


find_package(et-common-libs REQUIRED COMPONENTS cm-umode)
find_package(dnnLibrary REQUIRED)

add_library(etsoc_crt0 STATIC
  lib/deviceGpSdk.cc
  lib/crt0.S
)
target_include_directories(etsoc_crt0
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/../../common/include
)

target_compile_options(etsoc_crt0 PRIVATE -fno-exceptions -falign-functions=64 -O3 -g3)
target_compile_options(etsoc_crt0 PUBLIC -fno-threadsafe-statics -ftls-model=local-exec -fno-extern-tls-init)

target_link_libraries(etsoc_crt0 PUBLIC etgcc etm)

set_target_properties(etsoc_crt0 PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_link_libraries(etsoc_crt0
    PUBLIC
        et-common-libs::cm-umode
        libcpp_throw_stub
        libc_stub
)


#etgcc lib implements "softfloat" fp32 divs in HW by means of ET-SoC-1  extensions.
#by linking to this lib, -mno-fdiv option is propagated transparently to clients.
add_library(etgcc STATIC
  libetgcc/divsf.c
)
target_include_directories(etgcc
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/libetgcc/include
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
target_compile_options(etgcc PUBLIC -mno-fdiv)
endif()

#etm is just a subset of libm routines (Verbatim from newlib/libm) compiled with-mno-fdiv.
#libm will then be ganularily patched through wrap mechanism at link time.
add_library(etm STATIC
  libetm/w_pow.c
  libetm/w_log.c
  libetm/e_log.c
  libetm/w_acosh.c
  libetm/w_acos.c
  libetm/w_asin.c
  libetm/e_acosh.c 
  libetm/e_acos.c 
  libetm/e_asin.c
  libetm/e_atanh.c
  libetm/s_log1p.c
  libetm/e_pow.c
  libetm/wf_pow.c
  libetm/wf_log.c
  libetm/ef_log.c
  libetm/ef_acosh.c 
  libetm/ef_acos.c 
  libetm/wf_acosh.c
  libetm/wf_acos.c
  libetm/wf_asin.c
  libetm/ef_asin.c
  libetm/ef_atanh.c
  libetm/sf_log1p.c
  libetm/ef_pow.c
  libetm/ef_sqrtf.c
)
target_include_directories(etm
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/libetm/include
)

# Disable warning from code copied from newlib
target_compile_options(etm PRIVATE -Wno-undef -Wno-sign-conversion -Wno-return-type -Wno-sign-compare -Wno-conversion)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
target_compile_options(etm PUBLIC -mno-fdiv)
endif()

add_library(libcpp_throw_stub STATIC libcpp_throw_stub/functexcept.cc)
target_link_libraries(libcpp_throw_stub
    PRIVATE
        et-common-libs::cm-umode
)

add_library(libc_stub STATIC libc_stub/et_errno.cc libc_stub/et_atexit.cc)

add_subdirectory(libautogen)





