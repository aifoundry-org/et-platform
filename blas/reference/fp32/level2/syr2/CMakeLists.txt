add_etsoc_blas_kernel(
  blas_syr2_reference_fp32.elf
  syr2.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level2/syr2
)
target_include_directories(blas_syr2_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
