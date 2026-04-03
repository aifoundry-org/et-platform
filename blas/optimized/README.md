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
- `fp32/level1/dot`: scalar inner loop unrolled by 4 with independent
  accumulators, scalar tail handling, and the same cross-minion partial
  reduction contract as the reference kernel
- `fp32/level2/gemv`: output-row partitioning across minions with an unrolled
  inner reduction for both transpose modes
- `fp32/level3/gemm`: single-minion 4x4 blocked `NN` micro-kernel with K
  strip-mining for a bounded working set, plus reference-compatible fallbacks
  for the remaining transpose combinations

Current validation policy:

- prefer `*_dbg` kernel artifacts when validating optimized kernels
- treat release-ELF bring-up as a separate runtime/toolchain issue
- do not treat a release-ELF failure by itself as evidence that the optimized
  kernel math is wrong when the matching `_dbg` artifact passes
