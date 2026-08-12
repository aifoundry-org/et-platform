add_etsoc_blas_kernel(
  blas_swap_reference_fp32.elf
  swap.cpp
  INSTALL_DESTINATION kernels/blas/reference/fp32/level1/swap
)
target_include_directories(blas_swap_reference_fp32.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
