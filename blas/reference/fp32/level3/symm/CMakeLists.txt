add_etsoc_blas_kernel(
  blas_symm_reference_fp32.elf
  symm.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level3/symm
)
target_include_directories(blas_symm_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
