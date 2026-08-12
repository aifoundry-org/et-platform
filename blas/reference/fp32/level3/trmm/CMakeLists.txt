add_etsoc_blas_kernel(
  blas_trmm_reference_fp32.elf
  trmm.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level3/trmm
)
target_include_directories(blas_trmm_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
