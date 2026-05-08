#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------


find_package(et-common-libs REQUIRED COMPONENTS cm-umode erbium-umode erbium-soc1sim)
find_package(dnnLibrary REQUIRED)

add_library(etsoc_crt0 STATIC
  lib/deviceGpSdk.cc
  lib/crt0.S
)
target_include_directories(etsoc_crt0
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
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

#----------------------------------------------------------------------
# Erbium runtime archives (do-by-example boot/crt + linker script)
#----------------------------------------------------------------------
# Two STATIC archives that wrap the default M-mode boot, U-mode crt,
# and heap-region table for each erbium runtime backend. They mirror
# the role `etsoc_crt0` plays for the etsoc1 runtime: a kernel that
# wants its own `_boot`/`_start` simply defines them in its own
# sources -- the linker resolves the local definitions before
# scanning the archive, leaving these defaults out.
#
# Each archive publishes its companion linker script as the
# `ERBIUM_LINKER_SCRIPT` target property, so
# `add_erbium_riscv_executable` can read it off whichever backend
# was selected.

add_library(erbium_crt0 STATIC
  lib/erbium/boot.S
  lib/erbium/crt.S
  lib/erbium/layout.c
)
target_link_libraries(erbium_crt0 PUBLIC et-common-libs::erbium-umode)
set_target_properties(erbium_crt0 PROPERTIES
  ERBIUM_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/lib/erbium/erbium.ld"
)

add_library(erbium_soc1sim_crt0 STATIC
  lib/erbium-soc1sim/boot.S
  lib/erbium-soc1sim/crt.S
  lib/erbium-soc1sim/layout.c
)
target_link_libraries(erbium_soc1sim_crt0 PUBLIC et-common-libs::erbium-soc1sim)
set_target_properties(erbium_soc1sim_crt0 PROPERTIES
  ERBIUM_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/lib/erbium-soc1sim/erbium.ld"
)






