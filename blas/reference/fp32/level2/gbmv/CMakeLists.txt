add_etsoc_blas_kernel(
  blas_gbmv_reference_fp32.elf
  gbmv.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level2/gbmv
)
target_include_directories(blas_gbmv_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
