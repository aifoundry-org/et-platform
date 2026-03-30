# Reference Kernels

Reference kernels prioritize clear operation ordering and predictable numerical
behavior over peak throughput.

Allowed here:

- straightforward scalar implementations
- simple decompositions that preserve obvious operation order
- code written to maximize readability and auditability

Not allowed here:

- vector or tensor ISA use
- algorithmic rewrites done only for throughput
- numerically aggressive reordering meant to hide latency or reduce traffic
