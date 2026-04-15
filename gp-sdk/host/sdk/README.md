# GP-SDK Nested Scratchpad Quickstart

This is the shortest path from a freshly rebooted machine to a working ETSoC-1 demo.

If this works, you have:
- the card up
- the driver loaded
- GP-SDK host code built
- GP-SDK device code built
- the per-device nested-scratchpad topology inferred
- a real kernel running on silicon and using the inferred `16 MiB` nested scratchpad pool

## What You Are Running

The demo runs one neighborhood on one compute shire.

Around that compute shire, GP-SDK builds a per-device `nested star` scratchpad layout:
- 1 compute shire
- 4 relay shires
- 8 leaf shires that hold the data

That gives the kernel a logical `16 MiB` scratchpad pool.

The demo kernel:
- fills an `8 MiB` input tile
- runs a 5-point stencil
- writes an `8 MiB` output tile
- returns a checksum and sample pixels

The host launcher verifies the result.

## 1. Bring The Card Back After Reboot

From the repo root:

```bash
cd /home/lea/Developement/etsoc/et-platform
```

Check that the card is visible:

```bash
lspci -nn | rg '41:00.0|1e0a:eb01'
```

You want to see:

```text
41:00.0 Processing accelerators [1200]: Device [1e0a:eb01]
```

Try the driver first:

```bash
sudo modprobe et-soc1
```

If that fails because the module is missing for the current kernel, rebuild and install it:

```bash
cd et-driver
make dkms
cd ..
```

Now confirm the device nodes exist:

```bash
ls -l /dev/et0_mgmt /dev/et0_ops
```

You want both files to exist before doing anything else.

## 2. Build Only What You Need

Configure the device build:

```bash
cmake -S gp-sdk/device -B build/device-prefix/src/device-build -DADDRESS:STRING=0x8006335000
```

Configure the host build:

```bash
cmake -S gp-sdk/host -B build/host-prefix/src/host-build
```

Build the two device kernels and the one host launcher:

```bash
cmake --build build/device-prefix/src/device-build \
  --target shire_latency_probe.elf_dbg nested_scratchpad_stencil.elf_dbg \
  -j"$(nproc)"
```

```bash
cmake --build build/host-prefix/src/host-build \
  --target nested_scratchpad_stencil_demo \
  -j"$(nproc)"
```

## 3. Run The Demo

Run exactly this:

```bash
env \
  LD_LIBRARY_PATH=/home/lea/Developement/etsoc/et-platform/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib \
  ET_DISABLE_KERNEL_TRACES=1 \
  ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0 \
  ET_SKIP_MEMCPY_DEVICE_CHECK=1 \
  /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build/sdk/nested_scratchpad_stencil_demo \
    -d silicon \
    -k /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/nested_scratchpad_stencil.elf_dbg \
    -t 120 \
    --shire_mask=0x200 \
    --active_neighborhood=0 \
    --topology_probe_kernel=/home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/shire_latency_probe.elf_dbg
```

## 4. What Success Looks Like

The first run may take a bit longer because it builds a per-device topology cache.

A successful run looks like this:

```text
Loaded per-device topology cache from "/root/.cache/et/gpsdk/device-0-mask-ffffffff-spare-20.topology".
Topology cache: /root/.cache/et/gpsdk/device-0-mask-ffffffff-spare-20.topology
Nested center shire: 9
Relays: 1 24 25 0
Leaves: 13 17 16 4 2 10 8 3
loadKernel() kernel 0 loaded at 0x8006335000
Active threads: 8
Verified checksum: 0xffc26cb912401
Verified samples: top-left=0x66bd9d92 center=0x18952958 bottom-right=0xb06cd420
```

The exact shire numbers can change from card to card.

The important part is:
- the launcher resolves a topology
- the kernel runs
- the checksum is verified
- the sample pixels are verified

## 5. Run It Again

After the first run, the topology cache already exists, so the same command is usually faster.

If you want to force the topology to be re-measured:

```bash
env \
  LD_LIBRARY_PATH=/home/lea/Developement/etsoc/et-platform/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib \
  ET_DISABLE_KERNEL_TRACES=1 \
  ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0 \
  ET_SKIP_MEMCPY_DEVICE_CHECK=1 \
  /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build/sdk/nested_scratchpad_stencil_demo \
    -d silicon \
    -k /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/nested_scratchpad_stencil.elf_dbg \
    -t 120 \
    --shire_mask=0x200 \
    --active_neighborhood=0 \
    --topology_probe_kernel=/home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/shire_latency_probe.elf_dbg \
    --rebuild_topology_cache
```

## 6. If It Fails

Use these checks in order:

```bash
lspci -nn | rg '41:00.0|1e0a:eb01'
```

```bash
ls -l /dev/et0_mgmt /dev/et0_ops
```

```bash
lsmod | rg et_soc1
```

If the device nodes are missing, fix the driver first.

If the nodes exist but the demo still fails, rebuild the topology cache once with `--rebuild_topology_cache`.

If the runtime starts failing after previous bad runs, reboot the host and start again from step 1.

## Files Behind The Demo

- Host launcher: [nested_scratchpad_stencil_demo.cpp](./src/nested_scratchpad_stencil_demo/nested_scratchpad_stencil_demo.cpp)
- Device kernel: [nested_scratchpad_stencil.cc](../../device/tests/nested_scratchpad_stencil/nested_scratchpad_stencil.cc)
- Shared result format: [gpsdk_nested_scratchpad_example.h](../../common/include/gpsdk_nested_scratchpad_example.h)
