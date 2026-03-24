#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------

function(add_etsoc_test_kernel TARGET_NAME)
  set(options NO_INSTALL)
  set(oneValueArgs INSTALL_DESTINATION)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN} )

  if(NOT DEFINED ARGS_INSTALL_DESTINATION)
    set(ARGS_INSTALL_DESTINATION kernels)
  endif()

  add_etsoc_riscv_executable(${TARGET_NAME} ${ARGS_UNPARSED_ARGUMENTS})
  target_link_libraries(${TARGET_NAME}  etsoc_crt0)

  if (NOT ARGS_NO_INSTALL)
    install(TARGETS ${TARGET_NAME} ${TARGET_NAME}_dbg DESTINATION ${ARGS_INSTALL_DESTINATION})
  endif()
endfunction()

add_etsoc_test_kernel(print.elf print/print.cc)

add_etsoc_test_kernel(print2.elf print2/print2.cc)

add_etsoc_test_kernel(bss.elf kernelLaunch/resetBSS.cc)

add_etsoc_test_kernel(data.elf kernelLaunch/resetData.cc)

add_etsoc_test_kernel(hang.elf hang/hang.cc)

add_etsoc_test_kernel(exception.elf exception/exception.cc)

add_etsoc_test_kernel(exception4bt.elf exception/exception4bt.cc)

add_etsoc_test_kernel(saxpy_scalar.elf saxpy/saxpy.cpp)
target_include_directories(saxpy_scalar.elf INTERFACE .)

add_etsoc_test_kernel(saxpy_vector.elf saxpy/saxpy.cpp)
target_include_directories(saxpy_vector.elf INTERFACE .)
target_compile_definitions(saxpy_vector.elf PRIVATE "-DSAXPY_VECTOR")

add_etsoc_test_kernel(saxpy_intrinsics.elf saxpy/saxpy_intrinsics.cpp)
target_include_directories(saxpy_intrinsics.elf INTERFACE .)

add_etsoc_test_kernel(saxpy_profiling.elf saxpy/saxpy_profiling.cpp)
target_include_directories(saxpy_profiling.elf INTERFACE .)

add_etsoc_test_kernel(sdot_scalar.elf sdot/sdot.cpp)
target_include_directories(sdot_scalar.elf INTERFACE .)

add_etsoc_test_kernel(sdot_vector.elf sdot/sdot.cpp)
target_include_directories(sdot_vector.elf INTERFACE .)
target_compile_definitions(sdot_vector.elf PRIVATE "-DSDOT_VECTOR")

#FIXME: need to fix compiler errors
#add_etsoc_test_kernel(fftKernel.elf fftKernel/src/fft_entry.cc)
#target_link_libraries(fftKernel.elf dnnLibrary::dnn_lib)
#target_include_directories(fftKernel.elf PUBLIC fftKernel/include)
#target_compile_definitions(fftKernel.elf PRIVATE "-DGPSDK")

add_etsoc_test_kernel(syncDeviceBasic.elf barrier/syncDeviceBasic.cpp)

add_etsoc_test_kernel(syncMinion.elf barrier/syncMinion.cpp)

add_etsoc_test_kernel(syncAll.elf barrier/syncAll.cpp)

add_etsoc_test_kernel(syncShire2EP.elf barrier/syncShire2EP.cpp)

add_etsoc_test_kernel(txfma.elf txfma/txfma.cpp)
target_include_directories(txfma.elf INTERFACE .)

add_etsoc_test_kernel(oneTrapOnSync.elf trapping/oneTrapOnSync.cpp)

add_etsoc_test_kernel(busy10sec.elf busy10sec/busy10sec.cc)

#FIXME: need to fix compiler errors
#add_etsoc_test_kernel(check_pmc.elf check_pmc/check_pmc.cc)

#FIXME: need to fix compiler errors
#add_etsoc_test_kernel(fnodiv.elf softfloat/fnodiv.cpp)

add_etsoc_test_kernel(c_constructors.elf linker/c_constructors.cc)

add_etsoc_test_kernel(cpp_constructors.elf linker/cpp_constructors.cc)

add_etsoc_test_kernel(gp.elf linker/gp.cc)

add_etsoc_test_kernel(c_tls.elf linker/c_tls.cc)

add_etsoc_test_kernel(cpp_tls.elf linker/cpp_tls.cc)

add_etsoc_test_kernel(external_tls.elf linker/external_tls_def.cc linker/external_tls_decl.cc)

add_etsoc_test_kernel(profiling_simple.elf profiling/profiling_simple.cc)

add_etsoc_test_kernel(profiling_stress.elf profiling/profiling_stress.cc)

add_etsoc_test_kernel(cacheops_flush.elf cacheops/flush.cc)

add_etsoc_test_kernel(autogen_matmul.elf autogenMatmul/autogen_matmul.cpp)
target_link_libraries(autogen_matmul.elf matmul_f32_1024_512_512)

add_etsoc_test_kernel(fail_abort.elf fail/fail_abort.cc)

add_etsoc_test_kernel(fail_assert.elf fail/fail_assert.cc)

add_etsoc_test_kernel(variableStrings.elf variableStrings/variableStrings.cc)

add_etsoc_test_kernel(sysemu_fatal.elf sysemu_fatal/sysemu_fatal.cc)

add_etsoc_test_kernel(tracing_factorial.elf tracing/tracing_factorial.cc)

add_etsoc_test_kernel(tracing_busywait.elf tracing/tracing_busywait.cc)

add_etsoc_test_kernel(user_defined_stack.elf custom_stack/user_defined_stack.cc)

add_etsoc_test_kernel(hang4bt.elf hang/hang4bt.cc)

if($ENV{DEV_COMPILER} MATCHES "clang11")
  add_etsoc_test_kernel(exhaustive_cast.elf exhaustive_cast/exhaustive_cast.cpp)
endif()
