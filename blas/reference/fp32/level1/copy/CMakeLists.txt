add_etsoc_blas_kernel(
  blas_copy_reference_fp32.elf
  copy.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level1/copy
)
target_include_directories(blas_copy_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
