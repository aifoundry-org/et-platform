# GP-SDK Nested Scratchpad Quickstart

This is the shortest path from a freshly rebooted machine to a working ETSoC-1 demo.

If this works, you have:
- the card up
- the driver loaded
- GP-SDK host code built
- GP-SDK device code built
- the per-device nested-scratchpad topology inferred
- GP-SDK format-0 scratchpad accesses fenced to the selected cluster
- a real kernel running on silicon and using the inferred `16 MiB` nested scratchpad pool

## What You Are Running

The demo runs one neighborhood on one compute shire.

Around that compute shire, GP-SDK builds a per-device `nested star` scratchpad layout:
- 1 compute shire
- 4 relay shires
- 8 leaf shires that hold the data

That gives the kernel a logical `16 MiB` scratchpad pool.

The default demo kernel:
- fills an `8 MiB` input tile
- runs a 5-point stencil
- writes an `8 MiB` output tile
- returns a checksum and sample pixels

The host launcher verifies the result.

If you are using this path as an Erbium-equivalence aid, read [ERBIUM_SIM_PORTING_GUIDANCE.md](/home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/ERBIUM_SIM_PORTING_GUIDANCE.md) before treating it as hardware-faithful.

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

Build the base topology probe, the default stencil demo, the chimera GEMV proof-of-life, and the host launchers:

```bash
cmake --build build/device-prefix/src/device-build \
  --target shire_latency_probe.elf_dbg nested_scratchpad_stencil.elf_dbg nested_scratchpad_chimera_gemv.elf_dbg \
  -j"$(nproc)"
```

```bash
cmake --build build/host-prefix/src/host-build \
  --target nested_scratchpad_stencil_demo nested_scratchpad_chimera_gemv_demo \
  -j"$(nproc)"
```

If you also want the neighborhood-wide atomic-read validation, build these too:

```bash
cmake --build build/device-prefix/src/device-build \
  --target nested_scratchpad_atomic_reads.elf_dbg \
  -j"$(nproc)"
```

```bash
cmake --build build/host-prefix/src/host-build \
  --target nested_scratchpad_atomic_reads_demo \
  -j"$(nproc)"
```

If you also want the negative Erbium-sim fence validation, build these too:

```bash
cmake --build build/device-prefix/src/device-build \
  --target erbium_sim_access_violation.elf_dbg \
  -j"$(nproc)"
```

```bash
cmake --build build/host-prefix/src/host-build \
  --target erbium_sim_access_violation_demo \
  -j"$(nproc)"
```

If you also want the contiguous-window negative validation, build these too:

```bash
cmake --build build/device-prefix/src/device-build \
  --target star_scratchpad_boundary_violation.elf_dbg \
  -j"$(nproc)"
```

```bash
cmake --build build/host-prefix/src/host-build \
  --target star_scratchpad_boundary_violation_demo \
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
    --erbium_sim \
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
    --erbium_sim \
    --topology_probe_kernel=/home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/shire_latency_probe.elf_dbg \
    --rebuild_topology_cache
```

## 6. Atomic Read Validation

This validation keeps the same single active neighborhood on the center shire, then has all 8 active cores issue concurrent `atomic_load_global_64` reads into the 8 nested-star leaf shires.

Run it like this:

```bash
env \
  LD_LIBRARY_PATH=/home/lea/Developement/etsoc/et-platform/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib \
  ET_DISABLE_KERNEL_TRACES=1 \
  ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0 \
  ET_SKIP_MEMCPY_DEVICE_CHECK=1 \
  /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build/sdk/nested_scratchpad_atomic_reads_demo \
    -d silicon \
    -k /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/nested_scratchpad_atomic_reads.elf_dbg \
    -t 120 \
    --shire_mask=0x200 \
    --active_neighborhood=0 \
    --erbium_sim \
    --topology_probe_kernel=/home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/shire_latency_probe.elf_dbg
```

Success looks like:

```text
Active threads: 8
Verified aggregate checksum: 0x3d908e8dd6800000
Verified aggregate reads: 4096
Verified per-leaf reads: [0->shire 13: 512] [1->shire 17: 512] [2->shire 16: 512] [3->shire 4: 512] [4->shire 2: 512] [5->shire 10: 512] [6->shire 8: 512] [7->shire 3: 512]
```

The shire ids can vary by card. The important part is that:
- all 8 active threads participate
- the aggregate checksum matches
- each leaf shire receives the expected share of the remote atomic reads

## 7. Export The Resolved Scratchpad Map

Any launcher that goes through `GenericLauncher` can now export the resolved shard map for the selected scratchpad cluster:

```bash
--scratchpad_address_map=/tmp/nested_star_map.txt
```

For a nested-star run, the file records:
- requested and resolved shire masks
- center, relay, and auxiliary shires
- one line per `2 MiB` shard showing:
  - logical range in the exported pool
  - backing shire id
  - offset inside that shire
  - format-0 base and limit addresses

## 8. Contiguous Window Guard

The scratchpad pool is still segmented. It is not pointer-contiguous across shard boundaries.

The device-side helper now rejects a single contiguous request if it would cross a `2 MiB` shard boundary:
- `gpsdk::device::star_scratchpad::ptr<T>(logicalOffset, elementCount)`
- `gpsdk::device::star_scratchpad::window(logicalOffset, sizeBytes)`
- `gpsdk::device::star_scratchpad::maxContiguousBytes(logicalOffset)`

Use one of these patterns when staging tensor/general buffers:

```cpp
const auto bytes = rows * stride;
et_assert(bytes <= gpsdk::device::star_scratchpad::maxContiguousBytes(offset));
auto* base = gpsdk::device::star_scratchpad::ptr<std::byte>(offset, bytes);
```

or:

```cpp
const auto win = gpsdk::device::star_scratchpad::window(offset, bytes);
auto* base = reinterpret_cast<volatile std::byte*>(win.address);
```

That avoids silently allocating across two shires or leaking into per-shard guard space.

If you want a direct negative test for those two failure modes, run:

```bash
env \
  LD_LIBRARY_PATH=/home/lea/Developement/etsoc/et-platform/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib \
  ET_DISABLE_KERNEL_TRACES=1 \
  ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0 \
  ET_SKIP_MEMCPY_DEVICE_CHECK=1 \
  /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build/sdk/star_scratchpad_boundary_violation_demo \
    -d silicon \
    -k /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/star_scratchpad_boundary_violation.elf_dbg \
    -t 120 \
    --shire_mask=0x200 \
    --active_neighborhood=0 \
    --scratchpad_nested_star \
    --erbium_sim \
    --topology_probe_kernel=/home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/shire_latency_probe.elf_dbg
```

Expected result:

```text
Observed expected kernel failure for oversize_contiguous_request.
Observed expected kernel failure for cross_shard_contiguous_request.
```

## 9. Run The Supported Nested-Star Suite

This is the direct suite runner for the kernels and demos that are currently validated on silicon under:
- one active neighborhood
- inferred nested-star scratchpad topology
- `--erbium_sim` fencing

The long-lived status log for this suite is in [NESTED_STAR_SUITE_README.md](/home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/NESTED_STAR_SUITE_README.md).

Run it like this:

```bash
python3 -u /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_nested_star_suite.py
```

On this card, the current default passing set is:
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

If you want to see the full manifest, including currently unsupported cases and their reasons:

```bash
python3 /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_nested_star_suite.py --list
```

If you want to probe one of the currently skipped kernels anyway:

```bash
python3 -u /home/lea/Developement/etsoc/et-platform/gp-sdk/host/sdk/run_nested_star_suite.py \
  --include-skipped \
  --case saxpy_scalar
```

## 10. Chimera GEMV Proof Of Life

This is the mixed-unit proof.

It uses:
- nested-star scratchpad as the data tier
- SIMD loads and arithmetic for the full large GEMV
- tensor loads/FMA/store for a verified in-kernel tensor subproblem

Run it like this:

```bash
env \
  LD_LIBRARY_PATH=/home/lea/Developement/etsoc/et-platform/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib \
  ET_DISABLE_KERNEL_TRACES=1 \
  ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0 \
  ET_SKIP_MEMCPY_DEVICE_CHECK=1 \
  /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build/sdk/nested_scratchpad_chimera_gemv_demo \
    -d silicon \
    -k /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/nested_scratchpad_chimera_gemv.elf_dbg \
    -t 180 \
    --shire_mask=0x200 \
    --active_neighborhood=0 \
    --erbium_sim \
    --topology_probe_kernel=/home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/shire_latency_probe.elf_dbg
```

Success looks like:

```text
Active threads: 8
Verified GEMV checksum: -4.22852
Verified GEMV max abs diff: 0
Verified GEMV samples: first=0.720459 mid=-4.86719 last=-1.10986
Verified tensor checksum: -1.39551
Verified tensor max abs diff: 0
Verified tensor samples: first=-0.223633 mid=-1.17188
```

## 11. Erbium-Sim Fence Validation

This is the negative test.

It launches a kernel that intentionally targets format-0 scratchpad on a shire outside the selected cluster.

With `--erbium_sim`, all of these must fail:
- `atomic_load_global_32`
- `global_memcpy`
- `tensor_load`

### Star Cluster Example

```bash
env \
  LD_LIBRARY_PATH=/home/lea/Developement/etsoc/et-platform/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib \
  ET_DISABLE_KERNEL_TRACES=1 \
  ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0 \
  ET_SKIP_MEMCPY_DEVICE_CHECK=1 \
  /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build/sdk/erbium_sim_access_violation_demo \
    -d silicon \
    -k /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/erbium_sim_access_violation.elf_dbg \
    -t 60 \
    --shire_mask=0x200 \
    --active_neighborhood=0 \
    --scratchpad_star
```

Success looks like:

```text
Observed expected kernel failure for atomic_read at out-of-cluster shire ...
Observed expected kernel failure for global_memcpy at out-of-cluster shire ...
Observed expected kernel failure for tensor_load at out-of-cluster shire ...
```

### Block Cluster Example

```bash
env \
  LD_LIBRARY_PATH=/home/lea/Developement/etsoc/et-platform/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib \
  ET_DISABLE_KERNEL_TRACES=1 \
  ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0 \
  ET_SKIP_MEMCPY_DEVICE_CHECK=1 \
  /home/lea/Developement/etsoc/et-platform/build/host-prefix/src/host-build/sdk/erbium_sim_access_violation_demo \
    -d silicon \
    -k /home/lea/Developement/etsoc/et-platform/build/device-prefix/src/device-build/tests/erbium_sim_access_violation.elf_dbg \
    -t 60 \
    --shire_mask=0x200 \
    --active_neighborhood=0 \
    --scratchpad_block
```

This demo auto-enables `--erbium_sim`. If one of those accesses completes instead of faulting, the demo exits non-zero.

## 9. If It Fails

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
- Atomic-read launcher: [nested_scratchpad_atomic_reads_demo.cpp](./src/nested_scratchpad_atomic_reads_demo/nested_scratchpad_atomic_reads_demo.cpp)
- Chimera launcher: [nested_scratchpad_chimera_gemv_demo.cpp](./src/nested_scratchpad_chimera_gemv_demo/nested_scratchpad_chimera_gemv_demo.cpp)
- Chimera kernel: [nested_scratchpad_chimera_gemv.cc](../../device/tests/nested_scratchpad_chimera_gemv/nested_scratchpad_chimera_gemv.cc)
- Fence-validation launcher: [erbium_sim_access_violation_demo.cpp](./src/erbium_sim_access_violation_demo/erbium_sim_access_violation_demo.cpp)
- Fence-validation kernel: [erbium_sim_access_violation.cc](../../device/tests/erbium_sim_access_violation/erbium_sim_access_violation.cc)
- Atomic-read kernel: [nested_scratchpad_atomic_reads.cc](../../device/tests/nested_scratchpad_atomic_reads/nested_scratchpad_atomic_reads.cc)
- Atomic-read result format: [gpsdk_nested_scratchpad_atomic_reads.h](../../common/include/gpsdk_nested_scratchpad_atomic_reads.h)
