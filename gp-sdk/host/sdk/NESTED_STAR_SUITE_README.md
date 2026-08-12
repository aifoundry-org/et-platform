# Nested-Star Suite Status

This file is the running status log for the GP-SDK nested-star silicon suite.

Use it to track:
- which kernels are in the default supported suite
- which kernels are currently excluded and why
- what exact command was used to validate a given machine
- dated validation results over time
- the exported scratchpad map format and contiguous-window guard status
- the explicit negative test for oversize and cross-shard contiguous requests

## Suite Entry Points

Direct runner:

```bash
python3 -u /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_nested_star_suite.py
```

List the full manifest, including skipped cases:

```bash
python3 /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_nested_star_suite.py --list
```

Run one skipped case explicitly:

```bash
python3 -u /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_nested_star_suite.py \
  --include-skipped \
  --case saxpy_scalar
```

CI entry point:

```bash
pytest gp-sdk/ci/test_nested_star_examples.py \
  --with-gp-sdk /home/lea/Developement/etsoc/et-platform \
  --device-build /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build \
  --host-build /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build
```

Scratchpad map export for any direct launcher:

```bash
--scratchpad_address_map=/tmp/nested_star_map.txt
```

Contiguous-window helpers for device code:

```cpp
gpsdk::device::star_scratchpad::maxContiguousBytes(offset)
gpsdk::device::star_scratchpad::window(offset, bytes)
gpsdk::device::star_scratchpad::ptr<T>(offset, elementCount)
```

## Current Default Passing Set

These are the cases included in the default runner manifest today:

- `print`
- `print2`
- `bss`
- `data`
- `c_tls`
- `cpp_tls`
- `external_tls`
- `gp`
- `nested_scratchpad_stencil`
- `nested_scratchpad_atomic_reads`
- `nested_scratchpad_chimera_gemv`
- `erbium_sim_access_violation`

## Current Skipped Set

These are excluded from the default suite today and kept in the manifest with explicit reasons:

- `syncAll`: kernel barrier assumptions do not hold under single-neighborhood nested-star execution.
- `syncDeviceBasic`: barrier launcher aborts during kernel launch under nested-star silicon runs.
- `syncMinion`: barrier launcher aborts during kernel launch under nested-star silicon runs.
- `user_defined_stack`: custom per-hart stack setup is not yet compatible with nested-star execution.
- `saxpy_scalar`: kernel faults under nested-star single-neighborhood execution.
- `saxpy_vector`: kernel faults under nested-star single-neighborhood execution.
- `saxpy_intrinsics`: kernel faults under nested-star single-neighborhood execution.
- `sdot_scalar`: host runtime crashes in the D2H event path under nested-star execution.
- `sdot_vector`: host runtime crashes in the D2H event path under nested-star execution.
- `txfma`: kernel faults under nested-star single-neighborhood execution.
- `c_constructors`: faults under nested-star single-neighborhood silicon runs.
- `cpp_constructors`: faults under nested-star single-neighborhood silicon runs.
- `cacheops_flush`: hangs under nested-star silicon runs.
- `autogen_matmul`: not yet validated under single-shire nested-star execution.
- `profiling_simple`: profiling/tracing oriented; not part of the functional suite.
- `profiling_stress`: profiling/tracing oriented; not part of the functional suite.
- `tracing_busywait`: tracing validation is out of scope when kernel traces are disabled.
- `tracing_factorial`: tracing validation is out of scope when kernel traces are disabled.

## Validation Log

### 2026-04-16

Environment:
- branch: `feature/gpsdk-nested-scratchpad-demo`
- device mode: `silicon`
- shire mask: `0x200`
- active neighborhood: `0`
- scratchpad mode: `--scratchpad_nested_star`
- fencing: `--erbium_sim`
- topology probe: `build/device-prefix/src/device-build/tests/shire_latency_probe.elf_dbg`

Validated command:

```bash
python3 -u /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_nested_star_suite.py
```

Result:

```text
Summary: pass=12 fail=0 skip=0
```

Notes:
- The default suite passed end-to-end on this ETSOC1 card.
- The skipped set was also probed during bring-up and recorded in `gp-sdk/ci/nested_star_cases.py`.
- `saxpy_*`, `sdot_*`, `txfma`, barrier kernels, and `user_defined_stack` are not yet part of the supported nested-star set.
- `--scratchpad_address_map=/tmp/nested_star_map.txt` was validated on the stencil demo and exported the resolved 8-shard nested-star address map.
- The pool helper now rejects contiguous requests that would cross a `2 MiB` shard boundary.
- `star_scratchpad_boundary_violation_demo` is the explicit negative test for `> 2 MiB` contiguous requests and cross-shard contiguous requests.

## Updating This File

When rerunning on another card or after a runtime/kernel change:

1. Run the default suite.
2. If needed, probe individual skipped cases with `--include-skipped --case ...`.
3. Append a new dated entry under `Validation Log`.
4. Update the passing/skipped sections only if the supported default set actually changed.
