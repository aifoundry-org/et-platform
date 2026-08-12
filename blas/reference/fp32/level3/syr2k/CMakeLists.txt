add_etsoc_blas_kernel(
  blas_syr2k_reference_fp32.elf
  syr2k.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level3/syr2k
)
target_include_directories(blas_syr2k_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
