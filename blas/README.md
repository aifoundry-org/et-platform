# BLAS Kernels

This directory holds custom GP-SDK device kernels for the ET-SOC1 BLAS work.

It is intended to be consumed through the existing `gp-sdk/device` custom-kernel
hook by setting:

- `CUSTOM_KERNELS_SRC_DIR=<repo>/blas`
- `CUSTOM_KERNELS_BIN_DIR=<build-dir>/blas`

Layout conventions:

- `blas/reference/` holds numerically conservative scalar baselines
- `blas/optimized/` holds vector and tensor implementations
- datatype directories sit below those roots
- BLAS level and operation directories sit below the datatype

Datatype rollout order:

- `fp32` first
- `fp16`
- `bf16`
- `int16`
- `int8`

Implementation policy:

- every optimized kernel must have a matching reference kernel
- reference and optimized kernels must keep the same external argument contract
- numerical comparisons are always made against the reference implementation

The intent is to always keep a trustworthy reference kernel beside each
optimized family so we can measure error growth as operation ordering and
reduction strategies change.
