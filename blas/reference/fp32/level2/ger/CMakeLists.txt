add_etsoc_blas_kernel(
  blas_ger_reference_fp32.elf
  ger.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level2/ger
)
target_include_directories(blas_ger_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
