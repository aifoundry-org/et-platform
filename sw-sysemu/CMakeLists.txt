#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)
project(sw-sysemu VERSION 0.2.0)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

option(PROFILING "Enable profiling" OFF)
option(BACKTRACE "Enable backtrace" OFF)

include(GNUInstallDirs)

find_package(elfio REQUIRED)
find_package(glog REQUIRED)

# dependencies
if (BACKTRACE)
    find_package(libunwind REQUIRED)
endif()

add_library(sysemu
    sys_emu/checkers/flb_checker.cpp
    sys_emu/checkers/l1_scp_checker.cpp
    sys_emu/checkers/l2_scp_checker.cpp
    sys_emu/checkers/mem_checker.cpp
    sys_emu/gdbstub.cpp
    sys_emu/sys_emu.cpp
    sys_emu/sys_emu_main.cpp
    sys_emu/sys_emu_parse_args.cpp
    sys_emu/testLog.cpp
    sys_emu/utils.cpp
    devices/pcie_dma.cpp
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
    traps.cpp
    $<$<BOOL:${PROFILING}>:sys_emu/profiling.cpp>
    $<$<BOOL:${BACKTRACE}>:sys_emu/crash_handler.cpp>
)
add_library(sw-sysemu::sysemu ALIAS sysemu)
target_include_directories(sysemu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sys_emu>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(sysemu
    PUBLIC
        SYS_EMU
    PRIVATE
        $<$<BOOL:${BACKTRACE}>:BACKTRACE=1>
)
target_compile_features(sysemu PUBLIC cxx_std_17)
target_compile_options(sysemu PRIVATE -Wall -Wextra -pedantic-errors)
set_target_properties(sysemu PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(sysemu
    PRIVATE
        elfio::elfio
        $<$<BOOL:${BACKTRACE}>:libunwind::libunwind>
)

install(TARGETS sysemu
  EXPORT sw-sysemuTargets
  RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sw-sysemu
)

add_library(fpu
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
)
add_library(sw-sysemu::fpu ALIAS fpu)
target_include_directories(fpu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(fpu PUBLIC cxx_std_17)
target_compile_options(fpu PRIVATE -Wall -Werror -Wextra -Wno-implicit-fallthrough -Wno-sign-compare)
set_target_properties(fpu PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

install(TARGETS fpu
  EXPORT sw-sysemuTargets
  RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sw-sysemu
)

# sys_emu
add_executable(sys_emu
    sw-sysemu/main.cpp
)

install(TARGETS sys_emu
    RUNTIME DESTINATION ${SYSEMU_INSTALL_DIR}
    COMPONENT tools
)

target_link_libraries(sys_emu
    PRIVATE
        sw-sysemu::sysemu
        sw-sysemu::fpu
)

# sw-sysemu
add_library(sw-sysemu
    sw-sysemu/SysEmuImp.cpp
    sw-sysemu/SysEmuImp.h
    sw-sysemu/utils.h
    sw-sysemu/ISysEmu.cpp
)
add_library(sw-sysemu::sw-sysemu ALIAS sw-sysemu)
target_include_directories(sw-sysemu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sw-sysemu/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_compile_features(sw-sysemu PUBLIC cxx_std_17)
set_target_properties(sw-sysemu PROPERTIES
    PUBLIC_HEADER "sw-sysemu/include/sw-sysemu/ISysEmu.h;sw-sysemu/include/sw-sysemu/SysEmuOptions.h"
    POSITION_INDEPENDENT_CODE TRUE
    CXX_EXTENSIONS OFF
)
target_link_libraries(sw-sysemu
    PRIVATE
        sw-sysemu::sysemu
        sw-sysemu::fpu
        glog::glog
)

install(TARGETS sw-sysemu
    EXPORT sw-sysemuTargets
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sw-sysemu
)

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
