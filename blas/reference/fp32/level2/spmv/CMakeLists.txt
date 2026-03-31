add_etsoc_blas_kernel(
  blas_spmv_reference_fp32.elf
  spmv.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level2/spmv
)
target_include_directories(blas_spmv_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
