# Optimized Kernels

Optimized kernels hold vector and tensor implementations that are validated
against the matching reference kernels.

Allowed here:

- vector and tensor implementations
- tiling, blocking, fusion, and staging for performance
- controlled operation reordering when justified by performance

Required here:

- keep the same external kernel contract as the matching reference kernel
- document any intentional numerical tradeoff in code comments or commit history
- validate against the matching reference implementation

Not allowed here:

- silently changing kernel semantics
- changing datatype interpretation relative to the reference path
- landing an optimized kernel before a reference kernel exists

Current kernels:

- `fp32/level1/axpy`: scalar inner loop unrolled by 4 with the same minion
  partitioning and external contract as the reference kernel
- `fp32/level1/dot`: ET packed-SIMD loads plus packed multiply/add in the main
  loop, with a masked packed-SIMD tail in the default artifact and a scalar-tail
  comparison artifact kept alongside it
- `fp32/level2/gemv`: ET packed-SIMD vectorization across contiguous output-row
  blocks for the non-transpose, unit-stride path, with scalar fallback for
  transpose and non-unit-stride cases
- `fp32/level3/gemm`:
  - `blas_gemm_optimized_fp32_vector`: single-minion 4x4 blocked `NN`
    micro-kernel with K strip-mining for lower-latency execution
  - `blas_gemm_optimized_fp32_tensor`: tensor-engine tiled `NN` path for
    higher-throughput execution, with scalar fallback for unsupported tails and
    non-`NN` cases
  - current tensor bring-up keeps the tensor engine on the bulk of each tile
    and applies a post-store scalar correction for the dropped final inner term
    observed on the current hardware path

Current validation policy:

- prefer `*_dbg` kernel artifacts when validating optimized kernels
- treat release-ELF bring-up as a separate runtime/toolchain issue
- do not treat a release-ELF failure by itself as evidence that the optimized
  kernel math is wrong when the matching `_dbg` artifact passes

Current ET-SIMD bring-up notes:

- `fp32/level1/dot` now uses ET packed-SIMD loads and packed multiply/add in
  the main loop
- the default optimized `dot` artifact uses a masked packed-SIMD tail to avoid
  switching back into scalar FP arithmetic while packed state is still live in
  the overlaid `f` register file
- `blas_dot_optimized_fp32_scalar_tail.elf{,_dbg}` is retained as a comparison
  artifact for validating the scalar-tail alternative
