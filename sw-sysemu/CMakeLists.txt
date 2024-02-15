#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)
project(sw-sysemu VERSION 0.16.0 LANGUAGES ASM C CXX)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

option(BUILD_SHARED_LIBS "If enabled a shared library is built. Otherwise a static library is built." OFF)
option(CMAKE_POSITION_INDEPENDENT_CODE "By default set fPIC" ON)
if (BUILD_SHARED_LIBS AND NOT CMAKE_POSITION_INDEPENDENT_CODE)
    message(ERROR "Cannot build shared library without fPIC.")
endif()
set(VISIBILITY_PRESET_VALUES default protected hidden)
set(CMAKE_ASM_VISIBILITY_PRESET default CACHE STRING "Value for ASM symbol visibility compile flags. If ON, adds -fvisibility" FORCE)
set(CMAKE_C_VISIBILITY_PRESET   default CACHE STRING "Value for C   symbol visibility compile flags. If ON, adds -fvisibility" FORCE)
set(CMAKE_CXX_VISIBILITY_PRESET default CACHE STRING "Value for C++ symbol visibility compile flags. If ON, adds -fvisibility" FORCE)
set(CMAKE_VISIBILITY_INLINES_HIDDEN OFF CACHE BOOL "Whether to add a compile flag to hide symbols of inline functions. If ON, adds -fvisibility-inlines-hidden" FORCE)
set_property(CACHE CMAKE_ASM_VISIBILITY_PRESET PROPERTY STRINGS ${VISIBILITY_PRESET_VALUES})
set_property(CACHE CMAKE_C_VISIBILITY_PRESET PROPERTY STRINGS ${VISIBILITY_PRESET_VALUES})
set_property(CACHE CMAKE_CXX_VISIBILITY_PRESET PROPERTY STRINGS ${VISIBILITY_PRESET_VALUES})
if (NOT CMAKE_ASM_VISIBILITY_PRESET IN_LIST VISIBILITY_PRESET_VALUES)
    message(FATAL_ERROR "CMAKE_ASM_VISIBILITY_PRESET must be one of ${VISIBILITY_PRESET_VALUES}")
endif()
if (NOT CMAKE_C_VISIBILITY_PRESET IN_LIST VISIBILITY_PRESET_VALUES)
    message(FATAL_ERROR "CMAKE_C_VISIBILITY_PRESET must be one of ${VISIBILITY_PRESET_VALUES}")
endif()
if (NOT CMAKE_CXX_VISIBILITY_PRESET IN_LIST VISIBILITY_PRESET_VALUES)
    message(FATAL_ERROR "CMAKE_CXX_VISIBILITY_PRESET must be one of ${VISIBILITY_PRESET_VALUES}")
endif()

option(BENCHMARKS "Enable building benchmarks" OFF)
option(PROFILING "Enable profiling" OFF)
option(BACKTRACE "Enable backtrace" OFF)
option(PRELOAD_LZ4 " Enable lz4 compression for preloaded ELFs" OFF)
set(PRELOAD_ELFS "" CACHE STRING "Semicolon seperated list of ELFs to preload")
option(SDK_RELEASE "Enable various changes for SDK release" OFF)

option(ENABLE_IPO "Enable Inter-Procedural Optimizations (LTO)" OFF)
if (ENABLE_IPO)
    cmake_policy(SET CMP0069 NEW)

    include(CheckIPOSupported)
    check_ipo_supported(RESULT supports-ipo OUTPUT output)
    if (supports-ipo)
        message(STATUS "* IPO (Link time optimizations) is enabled for Release, RelWIthDebInfo and MinSizeRel builds.")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
        # LTO disabled for Debug
    else()
        message(WARNING "IPO is not supported: ${output}.")
    endif()
else()
    message(STATUS "* IPO is disabled.")
endif()

include(GNUInstallDirs)

find_package(elfio REQUIRED)
find_package(glog REQUIRED)
find_package(lz4 REQUIRED)

# dependencies
if (BACKTRACE)
    find_package(libunwind REQUIRED)
endif()


list(LENGTH PRELOAD_ELFS N_PRELOAD_ELFS)

configure_file(sys_emu/preload.cpp.in preload.cpp)

set(AUTOGEN_SRCS "${CMAKE_CURRENT_BINARY_DIR}/preload.cpp")

if (PRELOAD_ELFS)
    message(STATUS "Preloading ${N_PRELOAD_ELFS} ELFs")
    math(EXPR N_PRELOAD_ELFS_M1 "${N_PRELOAD_ELFS} - 1")
    foreach(i RANGE ${N_PRELOAD_ELFS_M1})
        list(GET PRELOAD_ELFS ${i} elf)
        message(STATUS "* preload[${i}]: ${elf}")
        if (PRELOAD_LZ4)
            add_custom_command(
                OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.elf.lz4
                COMMAND ${LZ4_EXECUTABLE} -f
                        ${elf}
                        ${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.elf.lz4
                DEPENDS ${elf}
            )
            add_custom_command(
                OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.S
                COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack.py
                        ${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.elf.lz4
                        ${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.S
                        g_preload_${i}
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack.py
                        ${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.elf.lz4
            )
        else()
            add_custom_command(
                OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.S
                COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack.py
                        ${elf}
                        ${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.S
                        g_preload_${i}
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack.py
                        ${elf}
            )
        endif()
        list(APPEND AUTOGEN_SRCS "${CMAKE_CURRENT_BINARY_DIR}/preload_${i}.S")
    endforeach()
endif()


if (SDK_RELEASE AND NOT BUILD_SHARED_LIBS)
    message(WARNING "Forcefully building a shared library for the SDK release")
    set(BUILD_SHARED_LIBS ON)
endif()


add_library(sw-sysemu)
add_library(sw-sysemu::sw-sysemu ALIAS sw-sysemu)
target_sources(sw-sysemu PRIVATE
    # Core sysemu files
    sys_emu/checkers/flb_checker.cpp
    sys_emu/checkers/l1_scp_checker.cpp
    sys_emu/checkers/l2_scp_checker.cpp
    sys_emu/checkers/mem_checker.cpp
    sys_emu/checkers/tstore_checker.cpp
    $<$<NOT:$<BOOL:${SDK_RELEASE}>>:sys_emu/checkers/vpurf_checker.cpp>
    sys_emu/gdbstub.cpp
    sys_emu/sys_emu.cpp
    sys_emu/sys_emu_main.cpp
    sys_emu/sys_emu_parse_args.cpp
    sys_emu/testLog.cpp
    sys_emu/utils.cpp
    sys_emu/log.cpp
    agent.cpp
    debugmodule.cpp
    devices/pcie_dma.cpp
    devices/spio_misc_region.cpp
    emu_gio.cpp
    esrs.cpp
    flb.cpp
    gold.cpp
    insns/arith.cpp
    insns/arith_atomic.cpp
    insns/arith_graphics.cpp
    insns/arith_loadstore.cpp
    insns/branch.cpp
    insns/c_arith.cpp
    insns/c_branch.cpp
    insns/c_loadstore.cpp
    insns/cache_control.cpp
    insns/coherent_arith_loadstore.cpp
    insns/coherent_packed_loadstore.cpp
    insns/float.cpp
    insns/float_loadstore.cpp
    insns/muldiv.cpp
    insns/packed_arith.cpp
    insns/packed_atomic.cpp
    insns/packed_float.cpp
    insns/packed_graphics.cpp
    insns/packed_loadstore.cpp
    insns/packed_mask.cpp
    insns/packed_trans.cpp
    insns/system.cpp
    insns/tensors.cpp
    insns/zicsr.cpp
    insns/zifencei.cpp
    memory/main_memory.cpp
    mmu.cpp
    msgport.cpp
    processor.cpp
    system.cpp
    $<$<BOOL:${PROFILING}>:sys_emu/profiling.cpp>
    $<$<BOOL:${BACKTRACE}>:sys_emu/crash_handler.cpp>
    ${AUTOGEN_SRCS}
    # Softfloat emulation
    fpu/cvt.cpp
    fpu/f10_to_f32.cpp
    fpu/f11_to_f32.cpp
    fpu/f32_cubeFaceIdx.cpp
    fpu/f32_cubeFaceSignS.cpp
    fpu/f32_cubeFaceSignT.cpp
    fpu/f32_frac.cpp
    fpu/f32_to_f10.cpp
    fpu/f32_to_f11.cpp
    fpu/f32_to_fxp1714.cpp
    fpu/fxp1516_to_f32.cpp
    fpu/fxp1714_rcpStep.cpp
    fpu/tensors.cpp
    fpu/ttrans.cpp
    fpu/f32_copySign.c
    fpu/f32_copySignNot.c
    fpu/f32_copySignXor.c
    fpu/f32_log2.c
    fpu/f32_maxNum.c
    fpu/f32_maximumNumber.c
    fpu/f32_minNum.c
    fpu/f32_minimumNumber.c
    fpu/f32_mulSub.c
    fpu/f32_subMulAdd.c
    fpu/f32_subMulSub.c
    softfloat/f128_add.c
    softfloat/f128_classify.c
    softfloat/f128_div.c
    softfloat/f128_eq.c
    softfloat/f128_eq_signaling.c
    softfloat/f128_isSignalingNaN.c
    softfloat/f128_le.c
    softfloat/f128_le_quiet.c
    softfloat/f128_lt.c
    softfloat/f128_lt_quiet.c
    softfloat/f128_mulAdd.c
    softfloat/f128_mul.c
    softfloat/f128_rem.c
    softfloat/f128_roundToInt.c
    softfloat/f128_sqrt.c
    softfloat/f128_sub.c
    softfloat/f128_to_f16.c
    softfloat/f128_to_f32.c
    softfloat/f128_to_f64.c
    softfloat/f128_to_i32.c
    softfloat/f128_to_i32_r_minMag.c
    softfloat/f128_to_i64.c
    softfloat/f128_to_i64_r_minMag.c
    softfloat/f128_to_ui32.c
    softfloat/f128_to_ui32_r_minMag.c
    softfloat/f128_to_ui64.c
    softfloat/f128_to_ui64_r_minMag.c
    softfloat/f16_add.c
    softfloat/f16_div.c
    softfloat/f16_eq.c
    softfloat/f16_eq_signaling.c
    softfloat/f16_isSignalingNaN.c
    softfloat/f16_le.c
    softfloat/f16_le_quiet.c
    softfloat/f16_lt.c
    softfloat/f16_lt_quiet.c
    softfloat/f16_mulAdd.c
    softfloat/f16_mul.c
    softfloat/f16_rem.c
    softfloat/f16_roundToInt.c
    softfloat/f16_sqrt.c
    softfloat/f16_sub.c
    softfloat/f16_to_f128.c
    softfloat/f16_to_f32.c
    softfloat/f16_to_f64.c
    softfloat/f16_to_i32.c
    softfloat/f16_to_i32_r_minMag.c
    softfloat/f16_to_i64.c
    softfloat/f16_to_i64_r_minMag.c
    softfloat/f16_to_ui32.c
    softfloat/f16_to_ui32_r_minMag.c
    softfloat/f16_to_ui64.c
    softfloat/f16_to_ui64_r_minMag.c
    softfloat/f32_add.c
    softfloat/f32_classify.c
    softfloat/f32_div.c
    softfloat/f32_eq.c
    softfloat/f32_eq_signaling.c
    softfloat/f32_isSignalingNaN.c
    softfloat/f32_le.c
    softfloat/f32_le_quiet.c
    softfloat/f32_lt.c
    softfloat/f32_lt_quiet.c
    softfloat/f32_mul.c
    softfloat/f32_mulAdd.c
    softfloat/f32_rem.c
    softfloat/f32_roundToInt.c
    softfloat/f32_sqrt.c
    softfloat/f32_sub.c
    softfloat/f32_to_f128.c
    softfloat/f32_to_f16.c
    softfloat/f32_to_f64.c
    softfloat/f32_to_i32.c
    softfloat/f32_to_i32_r_minMag.c
    softfloat/f32_to_i64.c
    softfloat/f32_to_i64_r_minMag.c
    softfloat/f32_to_ui32.c
    softfloat/f32_to_ui32_r_minMag.c
    softfloat/f32_to_ui64.c
    softfloat/f32_to_ui64_r_minMag.c
    softfloat/f64_add.c
    softfloat/f64_classify.c
    softfloat/f64_div.c
    softfloat/f64_eq.c
    softfloat/f64_eq_signaling.c
    softfloat/f64_isSignalingNaN.c
    softfloat/f64_le.c
    softfloat/f64_le_quiet.c
    softfloat/f64_lt.c
    softfloat/f64_lt_quiet.c
    softfloat/f64_mulAdd.c
    softfloat/f64_mul.c
    softfloat/f64_rem.c
    softfloat/f64_roundToInt.c
    softfloat/f64_sqrt.c
    softfloat/f64_sub.c
    softfloat/f64_to_f128.c
    softfloat/f64_to_f16.c
    softfloat/f64_to_f32.c
    softfloat/f64_to_i32.c
    softfloat/f64_to_i32_r_minMag.c
    softfloat/f64_to_i64.c
    softfloat/f64_to_i64_r_minMag.c
    softfloat/f64_to_ui32.c
    softfloat/f64_to_ui32_r_minMag.c
    softfloat/f64_to_ui64.c
    softfloat/f64_to_ui64_r_minMag.c
    softfloat/i32_to_f128.c
    softfloat/i32_to_f16.c
    softfloat/i32_to_f32.c
    softfloat/i32_to_f64.c
    softfloat/i64_to_f128.c
    softfloat/i64_to_f16.c
    softfloat/i64_to_f32.c
    softfloat/i64_to_f64.c
    softfloat/s_add128.c
    softfloat/s_add256M.c
    softfloat/s_addCarryM.c
    softfloat/s_addComplCarryM.c
    softfloat/s_addMagsF128.c
    softfloat/s_addMagsF16.c
    softfloat/s_addMagsF32.c
    softfloat/s_addMagsF64.c
    softfloat/s_addM.c
    softfloat/s_approxRecip_1Ks.c
    softfloat/s_approxRecip32_1.c
    softfloat/s_approxRecipSqrt_1Ks.c
    softfloat/s_approxRecipSqrt32_1.c
    softfloat/s_compare128M.c
    softfloat/s_compare96M.c
    softfloat/s_countLeadingZeros16.c
    softfloat/s_countLeadingZeros32.c
    softfloat/s_countLeadingZeros64.c
    softfloat/s_countLeadingZeros8.c
    softfloat/s_eq128.c
    softfloat/s_le128.c
    softfloat/s_lt128.c
    softfloat/s_mul128By32.c
    softfloat/s_mul128MTo256M.c
    softfloat/s_mul128To256M.c
    softfloat/s_mul64ByShifted32To128.c
    softfloat/s_mul64To128.c
    softfloat/s_mul64To128M.c
    softfloat/s_mulAddF128.c
    softfloat/s_mulAddF16.c
    softfloat/s_mulAddF32.c
    softfloat/s_mulAddF64.c
    softfloat/s_negXM.c
    softfloat/s_normRoundPackToF128.c
    softfloat/s_normRoundPackToF16.c
    softfloat/s_normRoundPackToF32.c
    softfloat/s_normRoundPackToF64.c
    softfloat/s_normSubnormalF128Sig.c
    softfloat/s_normSubnormalF16Sig.c
    softfloat/s_normSubnormalF32Sig.c
    softfloat/s_normSubnormalF64Sig.c
    softfloat/softfloat_raiseFlags.c
    softfloat/softfloat_state.c
    softfloat/s_propagateNaNF16UI.c
    softfloat/s_propagateNaNF32UI.c
    softfloat/s_propagateNaNF64UI.c
    softfloat/s_propagateNaNF128UI.c
    softfloat/s_remStepMBy32.c
    softfloat/s_roundMToI64.c
    softfloat/s_roundMToUI64.c
    softfloat/s_roundPackMToI64.c
    softfloat/s_roundPackMToUI64.c
    softfloat/s_roundPackToF128.c
    softfloat/s_roundPackToF16.c
    softfloat/s_roundPackToF32.c
    softfloat/s_roundPackToF64.c
    softfloat/s_roundPackToI32.c
    softfloat/s_roundPackToI64.c
    softfloat/s_roundPackToUI32.c
    softfloat/s_roundPackToUI64.c
    softfloat/s_roundToI32.c
    softfloat/s_roundToI64.c
    softfloat/s_roundToUI32.c
    softfloat/s_roundToUI64.c
    softfloat/s_shiftRightJam128.c
    softfloat/s_shiftRightJam128Extra.c
    softfloat/s_shiftRightJam256M.c
    softfloat/s_shiftRightJam32.c
    softfloat/s_shiftRightJam64.c
    softfloat/s_shiftRightJam64Extra.c
    softfloat/s_shortShiftLeft128.c
    softfloat/s_shortShiftLeft64To96M.c
    softfloat/s_shortShiftRight128.c
    softfloat/s_shortShiftRightExtendM.c
    softfloat/s_shortShiftRightJam128.c
    softfloat/s_shortShiftRightJam128Extra.c
    softfloat/s_shortShiftRightJam64.c
    softfloat/s_shortShiftRightJam64Extra.c
    softfloat/s_shortShiftRightM.c
    softfloat/s_sub128.c
    softfloat/s_sub1XM.c
    softfloat/s_sub256M.c
    softfloat/s_subMagsF128.c
    softfloat/s_subMagsF16.c
    softfloat/s_subMagsF32.c
    softfloat/s_subMagsF64.c
    softfloat/s_subM.c
    softfloat/ui32_to_f128.c
    softfloat/ui32_to_f16.c
    softfloat/ui32_to_f32.c
    softfloat/ui32_to_f64.c
    softfloat/ui64_to_f128.c
    softfloat/ui64_to_f16.c
    softfloat/ui64_to_f32.c
    softfloat/ui64_to_f64.c
    # SW wrapper
    sw-sysemu/SysEmuImp.cpp
    sw-sysemu/SysEmuImp.h
    sw-sysemu/utils.h
    sw-sysemu/ISysEmu.cpp
)
target_include_directories(sw-sysemu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/sw-sysemu/include/sw-sysemu>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sw-sysemu/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sys_emu>
)
target_compile_definitions(sw-sysemu
    PUBLIC
        SYS_EMU
    PRIVATE
        $<$<BOOL:${BACKTRACE}>:BACKTRACE=1>
        $<$<BOOL:${PRELOAD_LZ4}>:PRELOAD_LZ4=1>
        $<$<BOOL:${SDK_RELEASE}>:SDK_RELEASE=1>
)
target_compile_features(sw-sysemu PUBLIC cxx_std_17)
target_compile_options(sw-sysemu PRIVATE -Wall -Wextra -pedantic-errors -Werror -Wno-implicit-fallthrough -Wno-sign-compare)
set(SW_SYSEMU_PUBLIC_EXPORT_HEADER ${PROJECT_BINARY_DIR}/sw-sysemu/include/sw-sysemu/ISysEmuExport.hpp)
set_target_properties(sw-sysemu PROPERTIES
    PUBLIC_HEADER "sw-sysemu/include/sw-sysemu/ISysEmu.h;sw-sysemu/include/sw-sysemu/SysEmuOptions.h;${SW_SYSEMU_PUBLIC_EXPORT_HEADER}"
    CXX_EXTENSIONS OFF
)
include(GenerateExportHeader)
generate_export_header(sw-sysemu EXPORT_FILE_NAME ${SW_SYSEMU_PUBLIC_EXPORT_HEADER})
target_sources(sw-sysemu PRIVATE ${SW_SYSEMU_PUBLIC_EXPORT_HEADER})
target_link_libraries(sw-sysemu
    PRIVATE
        elfio::elfio
        $<$<BOOL:${BACKTRACE}>:libunwind::libunwind>
        lz4::lz4
        glog::glog
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0.0>>:stdc++fs>
)
target_link_options(sw-sysemu PRIVATE LINKER:-Bsymbolic)

if (SDK_RELEASE)
    add_custom_command(TARGET sw-sysemu POST_BUILD
            COMMAND strip -S -x $<TARGET_FILE:sw-sysemu>
            COMMENT "Stripping sw-sysemu symbols"
    )
    get_target_property(SW_SYSEMU_ASM_VISIBILITY_PRESET sw-sysemu ASM_VISIBILITY_PRESET)
    if (SW_SYSEMU_ASM_VISIBILITY_PRESET NOT STREQUAL "hidden")
        message(FATAL_ERROR "SDK builds must hide private symbols. Incorrect value for ASM_VISIBILITY_PRESET: ${SW_SYSEMU_ASM_VISIBILITY_PRESET}")
    endif()
    get_target_property(SW_SYSEMU_C_VISIBILITY_PRESET   sw-sysemu C_VISIBILITY_PRESET)
    if (SW_SYSEMU_C_VISIBILITY_PRESET NOT STREQUAL "hidden")
        message(FATAL_ERROR "SDK builds must hide private symbols. Incorrect value for C_VISIBILITY_PRESET: ${SW_SYSEMU_C_VISIBILITY_PRESET}")
    endif()
    get_target_property(SW_SYSEMU_CXX_VISIBILITY_PRESET sw-sysemu CXX_VISIBILITY_PRESET)
    if (SW_SYSEMU_CXX_VISIBILITY_PRESET NOT STREQUAL "hidden")
        message(FATAL_ERROR "SDK builds must hide private symbols. Incorrect value for CXX_VISIBILITY_PRESET: ${SW_SYSEMU_CXX_VISIBILITY_PRESET}")
    endif()
    get_target_property(SW_SYSEMU_VISIBILITY_INLINES_HIDDEN sw-sysemu VISIBILITY_INLINES_HIDDEN)
    if (NOT SW_SYSEMU_VISIBILITY_INLINES_HIDDEN)
        message(FATAL_ERROR "SDK builds must hide private symbols. Incorrect value for VISIBILITY_INLINES_HIDDEN: ${SW_SYSEMU_VISIBILITY_INLINES_HIDDEN}")
    endif()
endif()

install(TARGETS sw-sysemu
    EXPORT sw-sysemuTargets
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sw-sysemu
)

if (SDK_RELEASE)
    message(STATUS "SDK Build. Not building sys_emu executable.")
else()
    message(STATUS "Regular Build. Building sys_emu executable.")

    add_executable(sys_emu sw-sysemu/main.cpp)
    target_link_libraries(sys_emu PRIVATE sw-sysemu)
    target_include_directories(sys_emu
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sys_emu>
    )
    install(TARGETS sys_emu
        RUNTIME DESTINATION ${SYSEMU_INSTALL_DIR}
        COMPONENT tools
    )
endif()

if (BENCHMARKS)
    add_subdirectory(bench)
endif()

# Install the export set for use with the install-tree
install(EXPORT sw-sysemuTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sw-sysemu
    NAMESPACE sw-sysemu::
)


##### Package

include(CMakePackageConfigHelpers)

# Create sw-sysemuConfig.cmake
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sw-sysemuConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sw-sysemuConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sw-sysemu
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/sw-sysemuConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the sw-sysemuConfig.cmake and sw-sysemuConfigVersion.cmake
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sw-sysemuConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sw-sysemuConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sw-sysemu
)
