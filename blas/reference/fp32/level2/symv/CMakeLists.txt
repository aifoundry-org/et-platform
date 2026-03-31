add_etsoc_blas_kernel(
  blas_symv_reference_fp32.elf
  symv.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level2/symv
)
target_include_directories(blas_symv_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
