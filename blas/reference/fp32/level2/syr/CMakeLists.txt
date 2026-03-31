add_etsoc_blas_kernel(
  blas_syr_reference_fp32.elf
  syr.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level2/syr
)
target_include_directories(blas_syr_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
