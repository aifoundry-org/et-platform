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
