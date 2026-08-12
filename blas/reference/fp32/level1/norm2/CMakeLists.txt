add_etsoc_blas_kernel(
  blas_norm2_reference_fp32.elf
  norm2.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level1/norm2
)
target_include_directories(blas_norm2_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
