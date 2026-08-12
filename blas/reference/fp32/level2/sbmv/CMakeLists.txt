add_etsoc_blas_kernel(
  blas_sbmv_reference_fp32.elf
  sbmv.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level2/sbmv
)
target_include_directories(blas_sbmv_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
