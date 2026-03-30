# BLAS Kernels

This directory holds custom GP-SDK device kernels for the ET-SOC1 BLAS work.

It is intended to be consumed through the existing `gp-sdk/device` custom-kernel
hook by setting:

- `CUSTOM_KERNELS_SRC_DIR=<repo>/blas`
- `CUSTOM_KERNELS_BIN_DIR=<build-dir>/blas`

Current layout:

- `level1/axpy`: scalar single-precision AXPY kernel scaffold

The first goal here is a minimal, functional BLAS baseline. That means starting
with simple scalar kernels and a clean directory structure before adding vector
or tensor-specialized implementations.
