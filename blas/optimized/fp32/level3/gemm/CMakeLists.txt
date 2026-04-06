function(add_gemm_variant target_name source_file)
  add_etsoc_blas_kernel(
    ${target_name}
    ${source_file}
    INSTALL_DESTINATION kernels/blas/optimized/fp32/level3/gemm
  )
  target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

add_gemm_variant(blas_gemm_optimized_fp32_vector.elf gemm.cpp)
add_gemm_variant(blas_gemm_optimized_fp32_tensor.elf gemm_tensor.cpp)
