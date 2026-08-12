add_etsoc_blas_kernel(
  blas_asum_reference_fp32.elf
  asum.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level1/asum
)
target_include_directories(blas_asum_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
