#------------------------------------------------------------------------------
# Copyright (c) 2026 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------

function(add_etsoc_blas_kernel TARGET_NAME)
  set(options)
  set(oneValueArgs INSTALL_DESTINATION)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED ARGS_INSTALL_DESTINATION)
    set(ARGS_INSTALL_DESTINATION kernels/blas)
  endif()

  add_etsoc_riscv_executable(${TARGET_NAME} ${ARGS_UNPARSED_ARGUMENTS})
  target_link_libraries(${TARGET_NAME} etsoc_crt0)
  install(TARGETS ${TARGET_NAME} ${TARGET_NAME}_dbg DESTINATION ${ARGS_INSTALL_DESTINATION})
endfunction()

add_subdirectory(reference)
add_subdirectory(optimized)
