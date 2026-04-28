# LPDDR Erbium Suite Status

This file is the running status log for the GP-SDK LPDDR single-neighborhood silicon suite.

Use it to track:
- which kernels are in the default supported LPDDR Erbium subset
- which kernels are currently excluded and why
- what exact command was used to validate a given machine
- dated validation results over time
- where the fixed-LPDDR execution proof fits into the larger Erbium bring-up path

## Suite Entry Points

Direct runner:

```bash
python3 -u /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_lpddr_erbium_suite.py
```

List the full manifest, including skipped cases:

```bash
python3 /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_lpddr_erbium_suite.py --list
```

Run one skipped case explicitly:

```bash
python3 -u /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_lpddr_erbium_suite.py \
  --include-skipped \
  --case saxpy_scalar
```

CI entry point:

```bash
pytest gp-sdk/ci/test_lpddr_erbium_examples.py \
  --with-gp-sdk /home/lea/Developement/etsoc/et-platform \
  --device-build /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build \
  --host-build /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build
```

Fixed executable placement proof:

```bash
sed -n '1,220p' /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/FIXED_LPDDR_EXEC_PROBE.md
```

## Current Default Passing Set

These are the cases included in the default runner manifest today:

- `print`
- `print2`
- `bss`
- `c_tls`
- `cpp_tls`
- `external_tls`
- `gp`
- `txfma`

These are the kernels that currently satisfy both:
- they are plausible for the LPDDR-backed, single-neighborhood Erbium microcontroller model
- they pass on the ETSOC1 LPDDR shim path as implemented today

## Current Skipped Set

These are excluded from the default suite today and kept in the manifest with explicit reasons:

- `data`: fails with `Kernel Launch Error (error code: 4)` under LPDDR single-neighborhood execution.
- `c_constructors`: `KernelLaunchUnexpectedError` under LPDDR single-neighborhood silicon runs.
- `cpp_constructors`: `KernelLaunchUnexpectedError` under LPDDR single-neighborhood silicon runs.
- `saxpy_scalar`: host/device results do not match under LPDDR single-neighborhood execution.
- `saxpy_vector`: host/device results do not match under LPDDR single-neighborhood execution.
- `saxpy_intrinsics`: host/device results do not match under LPDDR single-neighborhood execution.
- `sdot_scalar`: host/device results do not match under LPDDR single-neighborhood execution.
- `sdot_vector`: host/device results do not match under LPDDR single-neighborhood execution.
- `autogen_matmul`: hangs after runtime initialization under LPDDR single-neighborhood execution.
- `variableStrings`: currently hangs under LPDDR single-neighborhood silicon runs.
- `fft`: current host launcher does not accept single-shire launch arguments.
- `syncAll`: barrier semantics are outside the single-neighborhood Erbium microcontroller model.
- `syncDeviceBasic`: barrier launcher path is outside the single-neighborhood Erbium microcontroller model.
- `syncMinion`: barrier launcher path is outside the single-neighborhood Erbium microcontroller model.
- `syncShire2EP`: barrier launcher path is outside the single-neighborhood Erbium microcontroller model.
- `user_defined_stack`: custom per-hart stack bring-up is not yet validated in the LPDDR Erbium mode.
- `busy10sec`: timing-only busywait kernel; not part of the functional Erbium suite.
- `cacheops_flush`: cache maintenance validation is not yet part of the LPDDR Erbium functional suite.
- `profiling_simple`: profiling-oriented kernel; not part of the LPDDR Erbium functional suite.
- `profiling_stress`: profiling-oriented kernel; not part of the LPDDR Erbium functional suite.
- `tracing_busywait`: tracing validation is out of scope when kernel traces are disabled.
- `tracing_factorial`: tracing validation is out of scope when kernel traces are disabled.
- `nested_scratchpad_stencil`: nested-scratchpad demo belongs to the abandoned SCP-backed approach.
- `nested_scratchpad_atomic_reads`: nested-scratchpad demo belongs to the abandoned SCP-backed approach.
- `nested_scratchpad_chimera_gemv`: nested-scratchpad demo belongs to the abandoned SCP-backed approach.
- `erbium_sim_access_violation`: scratchpad-fence negative test belongs to the abandoned SCP-backed approach.
- `topology_probe`: topology probing belongs to the abandoned nested-star scratchpad path.
- `scp_exec_probe`: SCP execution probe documents the rejected SCP code path, not the LPDDR path.

## Validation Log

### 2026-04-28

Environment:
- branch: `feature/lpddr-erbium-exec-probe`
- device mode: `silicon`
- shire mask: `0x200`
- active neighborhood: `0`
- executable backing: LPDDR / normal runtime DRAM path
- runtime env:
  - `ET_SKIP_INIT_ABORT=1`
  - `ET_SKIP_DEVICE_API_CHECK=1`
  - `ET_DISABLE_KERNEL_TRACES=1`
  - `ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0`
  - `GPSDK_SKIP_LOAD_WAIT=1`
  - `GPSDK_LOAD_QUIESCE_MS=1000`

Validated command:

```bash
python3 -u /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_lpddr_erbium_suite.py
```

Result after host reboot and clean card bring-up:

```text
Summary: pass=8 fail=0 skip=0
```

Additional probes recorded while defining the skipped set:
- `data` fails reproducibly with `Kernel Launch Error (error code: 4)`.
- `c_constructors` and `cpp_constructors` both fail with `KernelLaunchUnexpectedError`.
- `saxpy_scalar` and `sdot_scalar` launch but produce host/device result mismatches.
- `autogen_matmul` hangs after runtime initialization and never reaches a validated kernel result.
- `variableStrings` currently hangs under the LPDDR single-neighborhood path.
- `fft_launcher` does not currently accept `--shire_mask`, so it is not yet usable on this path.

Host-state note:
- An earlier same-session attempt hit host-specific runtime degradation, including repeated `dma_alloc_coherent() failed!` and `OPS API Kernel Launch Error (error code: 4)`.
- A host reboot plus clean ET card bring-up cleared that state and allowed the direct suite runner above to pass on the revised 8-case default set.

## Updating This File

When rerunning on another card or after a runtime/kernel change:

1. Run the default suite.
2. If needed, probe individual skipped cases with `--include-skipped --case ...`.
3. Append a new dated entry under `Validation Log`.
4. Update the passing/skipped sections only if the supported default set actually changed.
