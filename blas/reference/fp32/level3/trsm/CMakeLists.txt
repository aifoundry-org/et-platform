add_etsoc_blas_kernel(
  blas_trsm_reference_fp32.elf
  trsm.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level3/trsm
)
target_include_directories(blas_trsm_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
