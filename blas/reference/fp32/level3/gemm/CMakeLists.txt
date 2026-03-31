add_etsoc_blas_kernel(
  blas_gemm_reference_fp32.elf
  gemm.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level3/gemm
)
target_include_directories(blas_gemm_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
