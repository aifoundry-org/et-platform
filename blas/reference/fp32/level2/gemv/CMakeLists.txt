add_etsoc_blas_kernel(
  blas_gemv_reference_fp32.elf
  gemv.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level2/gemv
)
target_include_directories(blas_gemv_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
