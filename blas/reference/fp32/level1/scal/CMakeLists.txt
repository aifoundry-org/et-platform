add_etsoc_blas_kernel(
  blas_scal_reference_fp32.elf
  scal.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level1/scal
)
target_include_directories(blas_scal_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
