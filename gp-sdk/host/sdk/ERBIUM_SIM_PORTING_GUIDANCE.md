# Erbium-Sim Porting Guidance

This note is for developers using the GP-SDK `--erbium_sim` path on ETSOC1 as a proxy for future Erbium hardware.

Use it as a functional bring-up aid, not as proof of full hardware equivalence.

## What This Mode Is Good For

- validating kernels that use one active neighborhood on one compute shire
- exercising a constrained Erbium-like scratchpad tier built from surrounding shires
- catching out-of-cluster GP-SDK-managed scratchpad accesses
- proving that a kernel can run correctly against the nested-star logical pool

## What This Mode Is Not

- not an Erbium boot environment
- not an Erbium firmware persona
- not an Erbium MMIO model
- not an Erbium timing or bandwidth model
- not a guarantee that arbitrary raw ISA code will behave the same on taped-out Erbium

## Main Porting Risks

### 1. Segmented Scratchpad Pool

The exported scratchpad pool is logical, not raw-pointer contiguous.

- each shard contributes `2 MiB`
- larger contiguous requests are rejected
- requests that cross a shard boundary are rejected

That is a shim rule, not necessarily an Erbium architectural rule.

Code written around:

- `gpsdk::device::star_scratchpad::window(...)`
- `gpsdk::device::star_scratchpad::ptr<T>(offset, count)`
- `gpsdk::device::star_scratchpad::maxContiguousBytes(...)`

is portable within this shim, but may be more restrictive than real Erbium needs.

### 2. Per-Device Topology Inference

Nested-star placement is inferred from measured latency and cached per card.

That means:

- shire ids can differ across ETSOC1 cards
- relay and leaf choices are not a stable architectural contract
- code must not hard-code inferred center/relay/leaf ids

Real Erbium should have an architectural topology definition. This shim does not.

### 3. Reduced Concurrency

This path intentionally limits execution to one neighborhood.

That can hide bugs in:

- barriers
- collectives
- wakeups
- partitioning logic
- occupancy-sensitive code

Passing here does not mean full-chip Erbium concurrency is correct.

### 4. Software Fence vs Hardware Enforcement

`--erbium_sim` is a GP-SDK software fence.

It checks GP-SDK-managed accesses such as:

- scratchpad pool helpers
- `global_memcpy` / `global_memset`
- wrapped global atomics
- wrapped tensor memory ops

It is not a hardware protection mechanism. Code that bypasses the wrappers can diverge from the shim rules.

### 5. Timing and Contention Are ETSOC1 Timing

Even in nested-star mode, the underlying NoC, tensor path, atomics, and contention behavior are still ETSOC1 behavior.

Treat this as:

- useful for functional validation

Do not treat it as:

- cycle-accurate Erbium validation
- bandwidth validation
- scaling validation
- contention validation

### 6. Low-Level Runtime Surface Is Different

This path does not validate:

- boot flow
- traps
- system registers
- interrupt routing
- MMIO behavior
- firmware interaction

If your software depends on those, this shim is not the right equivalence target.

## Recommended Developer Discipline

If you want this path to help, not hurt, later Erbium bring-up:

- keep scratchpad access behind a small abstraction layer
- do not hard-code discovered shire ids
- treat `--erbium_sim` failures as shim diagnostics, not architectural truth
- revalidate tensor, atomic, and synchronization-heavy kernels on real Erbium early
- avoid using shim-specific limits, such as the `2 MiB` contiguous rule, as if they were Erbium requirements

## Good Uses

- early kernel bring-up
- checking that code respects a constrained scratchpad cluster
- proving correctness of nested-star data placement
- catching obvious illegal SCP accesses before Erbium tapeout

## Bad Uses

- claiming performance parity with Erbium
- claiming MMIO or firmware compatibility
- hard-coding topology learned from one ETSOC1 card
- treating the shim’s segmented pool rules as final Erbium memory rules

## Bottom Line

This mode is best understood as:

- a practical Erbium-like software target for a restricted class of GP-SDK kernels

It is not:

- a full hardware-equivalence environment for taped-out Erbium
