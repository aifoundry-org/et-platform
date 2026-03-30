add_etsoc_blas_kernel(blas_axpy_scalar.elf axpy.cpp)
target_include_directories(blas_axpy_scalar.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
