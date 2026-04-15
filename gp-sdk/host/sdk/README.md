# GP-SDK Scratchpad Cluster Modes

`GenericLauncher` supports two host-side launch shims for turning auxiliary ETSOC1 shires into a larger scratchpad-backed pool around a single compute shire.

Both modes are intended to be used with:
- `--active_neighborhood=<0-3>`
- a one-bit `--shire_mask=<center>`

The selected center shire must not be on the edge of the current `4x8` compute-shire mesh.

## `--scratchpad_star`

Expands the one-bit compute shire into a 5-shire cluster:
- center
- north
- south
- east
- west

Behavior:
- only the selected neighborhood in the center shire is compute-active
- the four auxiliary shires are scratchpad-only
- exports an `8 MiB` logical scratchpad pool (`4 * 2 MiB`)

Example:

```bash
basic_launcher -d silicon -k <kernel>.elf_dbg \
  --shire_mask=0x200 \
  --active_neighborhood=0 \
  --scratchpad_star
```

## `--scratchpad_block`

Expands the one-bit compute shire into a 9-shire surround block:
- center
- north, north-east, east, south-east
- south, south-west, west, north-west

Behavior:
- only the selected neighborhood in the center shire is compute-active
- the eight auxiliary shires are scratchpad-only
- exports a `16 MiB` logical scratchpad pool (`8 * 2 MiB`)

Example:

```bash
basic_launcher -d silicon -k <kernel>.elf_dbg \
  --shire_mask=0x200 \
  --active_neighborhood=0 \
  --scratchpad_block
```

## Validation

The reusable proof launcher is:

```bash
star_scratchpad_proof -d silicon -k <kernel>.elf_dbg \
  --shire_mask=0x200 \
  --active_neighborhood=0 \
  --scratchpad_star
```

or:

```bash
star_scratchpad_proof -d silicon -k <kernel>.elf_dbg \
  --shire_mask=0x200 \
  --active_neighborhood=0 \
  --scratchpad_block
```

For direct probe reads in the star case, add:

```bash
--read_neighbor_probes
```
