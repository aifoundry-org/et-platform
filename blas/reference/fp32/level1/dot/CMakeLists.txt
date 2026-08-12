add_etsoc_blas_kernel(
  blas_dot_reference_fp32.elf
  dot.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level1/dot
)
target_include_directories(blas_dot_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
