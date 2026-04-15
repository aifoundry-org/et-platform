# GP-SDK Scratchpad Cluster Modes

`GenericLauncher` supports three host-side launch shims for turning auxiliary ETSOC1 shires into a larger scratchpad-backed pool around a single compute shire.

Both modes are intended to be used with:
- `--active_neighborhood=<0-3>`
- a one-bit `--shire_mask=<center>`

For `--scratchpad_star` and `--scratchpad_block`, the selected center shire must not be on the edge of the current `4x8` compute-shire mesh.

For `--scratchpad_nested_star`, the launcher resolves an effective center against the device's active compute-shire mask. On yielded parts, it may slide the requested center to the nearest viable nested-star center so the full `13`-shire cluster remains intact.

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

## `--scratchpad_nested_star`

Expands the requested center into a 13-shire nested star:
- center compute shire
- four cardinal relay shires
- eight equal-distance leaf shires that back the exported pool

Behavior:
- only the selected neighborhood in the effective center shire is compute-active
- the four relay shires are scratchpad-only transit shires
- the eight leaf shires provide the actual exported pool
- exports a `16 MiB` logical scratchpad pool (`8 * 2 MiB`) with uniform two-hop distance from the center
- on yielded parts, the launcher may shift the effective center to the nearest viable active center

Example:

```bash
basic_launcher -d silicon -k <kernel>.elf_dbg \
  --shire_mask=0x4000 \
  --active_neighborhood=0 \
  --scratchpad_nested_star
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

or:

```bash
star_scratchpad_proof -d silicon -k <kernel>.elf_dbg \
  --shire_mask=0x4000 \
  --active_neighborhood=0 \
  --scratchpad_nested_star
```

For direct probe reads in the star case, add:

```bash
--read_neighbor_probes
```
