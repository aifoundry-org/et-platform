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

The combined host entry point for validation is
`blas_reference_full_verifier`, which writes a structured markdown result tree
plus a summary file.

Verification policy:

- small problems should be checked across host, `sysemu`, and `silicon`
- larger problems should be checked across host and `silicon`
- the full verifier enforces this with per-level `sysemu` size limits
- the default policy keeps `sysemu` enabled for Level 3 only up to
  `problem_dim=128`; larger Level 3 runs are recorded as policy skips in the
  summary
- for `silicon`, `kernel_launch_timeout` is a minimum, not a fixed cap
- the full verifier automatically raises timeouts for larger workloads,
  especially Level 3, so large `O(N^3)` kernels are not misclassified as
  failures just because the small-problem watchdog was too short
