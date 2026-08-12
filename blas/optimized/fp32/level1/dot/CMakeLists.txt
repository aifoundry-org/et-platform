function(add_dot_variant target_name tail_define)
  add_etsoc_blas_kernel(
    ${target_name}
    dot.cpp
    INSTALL_DESTINATION kernels/blas/optimized/fp32/level1/dot
  )
  target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(${target_name} PRIVATE ${tail_define}=1)
endfunction()

add_dot_variant(blas_dot_optimized_fp32.elf ET_BLAS_DOT_TAIL_MASKED)
add_dot_variant(blas_dot_optimized_fp32_scalar_tail.elf ET_BLAS_DOT_TAIL_SCALAR)
