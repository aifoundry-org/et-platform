add_etsoc_blas_kernel(
  blas_syrk_reference_fp32.elf
  syrk.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level3/syrk
)
target_include_directories(blas_syrk_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
