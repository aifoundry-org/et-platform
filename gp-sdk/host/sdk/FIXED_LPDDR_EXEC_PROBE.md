# Fixed LPDDR Exec Probe

This proof checks the specific thing the SCP path could not:

- load a kernel into a chosen fixed LPDDR/DRAM subregion on ETSOC1
- launch it through the normal runtime path
- verify that the kernel actually executes from that fixed address

It does **not** emulate Erbium yet. It only proves that executable placement in LPDDR is viable, which is the next useful substrate for an Erbium shim.

The follow-on functional validation suite for this LPDDR path is tracked in
[LPDDR_ERBIUM_SUITE_README.md](/home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/LPDDR_ERBIUM_SUITE_README.md).

## What Is Checked

The device probe kernel writes back:

- `started`
- `completed`
- `observedEntryAddress = &entryPoint`

The host launcher parses the ELF, resolves the probe `entryPoint` symbol address, launches the kernel, and checks:

- the ELF fixed load base is inside device DRAM
- the observed device entry address matches the fixed ELF entry-point symbol

## Host Target

The host-side launcher target is:

```bash
cmake --build build/host-prefix/src/host-build --target fixed_lpddr_exec_probe_demo -j 8
```

## Fixed-Address Device Kernel

Build the probe kernel at a fixed LPDDR address with:

```bash
bash gp-sdk/device/tests/scp_exec_probe/build_fixed_lpddr_probe.sh
```

Defaults:

- build dir: `/tmp/fixed_lpddr_probe_device_build`
- fixed base: `0x87fee00000`

You can override both:

```bash
bash gp-sdk/device/tests/scp_exec_probe/build_fixed_lpddr_probe.sh /tmp/my_probe_build 0x87fee00000
```

The resulting kernel is:

```text
/tmp/fixed_lpddr_probe_device_build/tests/scp_exec_probe.elf_dbg
```

## Validated Silicon Run

This is the command that passed on the ETSOC1 host used for bring-up:

```bash
LD_LIBRARY_PATH=/home/lea/Developement/etsoc/et-platform/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib \
ET_SKIP_INIT_ABORT=1 \
ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0 \
ET_DISABLE_KERNEL_TRACES=1 \
/home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build/sdk/fixed_lpddr_exec_probe_demo \
  -d silicon \
  -k /tmp/fixed_lpddr_probe_device_build/tests/scp_exec_probe.elf_dbg \
  -m 0x200 \
  --active_neighborhood=0 \
  -t 120
```

## Expected Success Signal

The key lines are:

```text
loadKernel() kernel 0 loaded at 0x87fee00000
Local DRAM base: 0x8005801000
Fixed ELF load base: 0x87fee01000
Expected entry address: 0x87fee01040
Observed entry address: 0x87fee01040
Fixed LPDDR exec probe passed.
```

Interpretation:

- runtime reserved and loaded the absolute-address ELF into DRAM
- the probe kernel actually executed from the fixed LPDDR region
- the observed `entryPoint` PC matched the fixed-address ELF symbol

## Notes

- The fixed ELF base used in validation, `0x87fee01000`, was inside the ETSOC1 DRAM window reported by runtime.
- The `_dbg` ELF is the one to use here, because the device build links debug ELFs at `ADDRESS + 0x1000`.
- The probe uses the existing `scp_exec_probe` kernel payload; only the placement changes.
- This proof depends on the same runtime env workarounds that were already needed on this host for stable silicon bring-up.
