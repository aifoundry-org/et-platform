add_etsoc_blas_kernel(
  blas_axpy_optimized_fp32.elf
  axpy.cpp
  INSTALL_DESTINATION kernels/blas/optimized/fp32/level1/axpy
)
target_include_directories(blas_axpy_optimized_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
