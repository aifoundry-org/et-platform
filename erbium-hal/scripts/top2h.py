#!/usr/bin/env python3
"""Generate a top-level address header from a top RDL file.

The input is a top-level memory-map style SystemRDL file such as
`top_cpu_mm.rdl`, where the important information is the placement of block
instances:

    System_Reg system_registers @0x2000000;
    UART_Reg   uart_registers   @0x2004000;
    Erbium_ESR cpu_registers    @0x80000000;

This script intentionally does not elaborate the full SystemRDL model. The top
maps often reference optional submodule RDLs that may be absent in a partial
checkout. Instead, it text-parses the top file and any available `include`d
RDLs to recover the address placements we need.

Each placement is emitted as <PREFIX>_<INSTANCE>_BASE. When the source text
exposes enough information, <PREFIX>_<INSTANCE>_SIZE and _END macros are
emitted too. Sizes are resolved in this order for each placement's kind:

  1. mem blocks: mementries * memwidth/8 (explicit in RDL)
  2. named reg types (`reg <Name> { ... };`): regwidth/8 bytes
  3. addrmap / regfile containers: recursive max(address + child_size)
     across sub-placements and anonymous reg children
  4. no size emitted (kind not found or its content not parseable)
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import re
import sys
from pathlib import Path


# SystemRDL has two container keywords with near-identical structure for our
# purposes: `addrmap` (top-level memory map) and `regfile` (named group of
# registers instantiable inside another map). Both can contain reg defs, reg
# instances, placements, alignment and regwidth. We treat them the same.
ADDRMAP_RE = re.compile(r"^\s*(?:addrmap|regfile)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{")
ADDR_RE = re.compile(r"@\s*(0x[0-9A-Fa-f_]+|[0-9_]+)")
INCLUDE_RE = re.compile(r'^\s*`include\s+"([^"]+)"')
MEM_RE = re.compile(r"^\s*mem\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{")
MENTRIES_RE = re.compile(r"\bmementries\s*=\s*(0x[0-9A-Fa-f_]+|[0-9_]+)\s*;")
MEMWIDTH_RE = re.compile(r"\bmemwidth\s*=\s*(0x[0-9A-Fa-f_]+|[0-9_]+)\s*;")
# Anonymous reg inside an addrmap body: `reg` followed by `{` (no type name).
# We detect the `reg {` opener and then wait for the matching `}` at the
# original depth to find the trailing `Name [@ offset];`.
REG_OPEN_RE = re.compile(r"^\s*reg\s*(\{|$)")
REG_CLOSE_RE = re.compile(
    r"\}\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:@\s*(0x[0-9A-Fa-f_]+|[0-9_]+))?\s*;"
)
# Named reg type definition at file scope: `reg <TypeName> { ... };`. Instances
# of it appear later as placements: `TypeName instance @ offset;`.
REG_TYPE_OPEN_RE = re.compile(r"^\s*reg\s+([A-Za-z_][A-Za-z0-9_]*)\s*(\{|$)")
ALIGN_RE = re.compile(r"^\s*alignment\s*=\s*(0x[0-9A-Fa-f_]+|[0-9_]+)\s*;")
DEFAULT_REGWIDTH_RE = re.compile(
    r"^\s*default\s+regwidth\s*=\s*(0x[0-9A-Fa-f_]+|[0-9_]+)\s*;"
)
REGWIDTH_RE = re.compile(r"\bregwidth\s*=\s*(0x[0-9A-Fa-f_]+|[0-9_]+)\s*;")


@dataclass(frozen=True)
class Placement:
    kind: str
    instance: str
    export_name: str
    address: int


@dataclass(frozen=True)
class MemDef:
    name: str
    size: int


@dataclass(frozen=True)
class RegDef:
    name: str
    offset: int | None   # explicit `@ 0xN` offset, or None for sequential


@dataclass
class AddrmapDef:
    """Content of a parsed addrmap body relevant to size computation."""
    name: str
    placements: list["Placement"]    # sub-addrmap instances with @ offsets
    regs: list[RegDef]               # reg definitions inside this addrmap
    alignment: int                   # `alignment = N;` or default 4
    regwidth: int                    # `default regwidth = W;` bits or default 32


@dataclass(frozen=True)
class RegTypeDef:
    """Named reg type defined at file scope: `reg <name> { ... };`.
    Instances of it are placed in addrmaps via `<name> <instance> @ offset;`."""
    name: str
    regwidth: int  # bits


def hex_literal(value: int) -> str:
    if value <= 0xFFFFFFFF:
        return f"0x{value:08X}ul"
    return f"0x{value:016X}ull"


def sanitize_lines(path: Path) -> list[str]:
    return [
        raw_line.split("//", 1)[0].strip()
        for raw_line in path.read_text(encoding="utf-8").splitlines()
    ]


def parse_int(token: str) -> int:
    return int(token.replace("_", ""), 0)


def parse_available_includes(root: Path) -> list[Path]:
    seen: set[Path] = set()
    ordered: list[Path] = []

    def walk(path: Path) -> None:
        path = path.resolve()
        if path in seen or not path.exists():
            return
        seen.add(path)
        ordered.append(path)
        for line in sanitize_lines(path):
            m = INCLUDE_RE.match(line)
            if not m:
                continue
            inc = (path.parent / m.group(1)).resolve()
            if inc.exists():
                walk(inc)

    walk(root)
    return ordered


def parse_placement_line(line: str) -> Placement | None:
    if "@" not in line or ";" not in line:
        return None

    left, _ = line.split("@", 1)
    tokens = left.strip().split()
    if not tokens or tokens[0] == "property":
        return None

    addr_match = ADDR_RE.search(line)
    if not addr_match:
        return None

    instance = tokens[-1]
    kind = " ".join(tokens[:-1])
    export_name = tokens[1] if tokens[0] == "external" and len(tokens) >= 3 else instance
    address = parse_int(addr_match.group(1))
    return Placement(kind=kind, instance=instance, export_name=export_name, address=address)


def parse_addrmap_defs(path: Path) -> tuple[dict[str, AddrmapDef], dict[str, RegTypeDef]]:
    """Parse an RDL file and return (addrmaps, reg_types). Captures:
      - addrmap sub-addrmap placements (Type instance @ offset;)
      - addrmap anonymous reg definitions (reg { ... } Name [@ offset];)
      - addrmap alignment and default regwidth properties
      - file-scope named reg type definitions (reg <Name> { ... };)
    Uses brace-depth bookkeeping to handle multi-line bodies and nested
    field { ... } blocks."""
    addrmaps: dict[str, AddrmapDef] = {}
    reg_types: dict[str, RegTypeDef] = {}

    cur_name: str | None = None
    cur_placements: list[Placement] = []
    cur_regs: list[RegDef] = []
    cur_align = 4
    cur_regwidth = 32
    depth = 0                       # brace depth relative to file start
    addrmap_open_depth = -1         # depth at which the current addrmap opened
    reg_open_depth: int | None = None  # depth at which an anon reg body opened

    # File-scope named reg type currently being parsed.
    file_regwidth = 32               # file-scope `default regwidth = W;`
    cur_regtype: str | None = None
    regtype_open_depth = -1
    regtype_width = 32

    for line in sanitize_lines(path):
        if not line:
            continue

        opens = line.count("{")
        closes = line.count("}")

        # --- File scope ---
        if cur_name is None and cur_regtype is None:
            m = ADDRMAP_RE.match(line)
            if m:
                cur_name = m.group(1)
                cur_placements = []
                cur_regs = []
                cur_align = 4
                cur_regwidth = file_regwidth
                addrmap_open_depth = depth
                depth += opens - closes
                continue

            m = REG_TYPE_OPEN_RE.match(line)
            if m:
                cur_regtype = m.group(1)
                regtype_open_depth = depth
                regtype_width = file_regwidth
                depth += opens - closes
                if depth <= regtype_open_depth:
                    # Whole `reg Name { ... };` on one line
                    reg_types[cur_regtype] = RegTypeDef(
                        name=cur_regtype, regwidth=regtype_width)
                    cur_regtype = None
                continue

            m = DEFAULT_REGWIDTH_RE.match(line)
            if m:
                file_regwidth = parse_int(m.group(1))

            depth += opens - closes
            continue

        # --- Inside a named reg type body (file scope) ---
        if cur_regtype is not None:
            # Track regwidth overrides inside the reg body (not `default`).
            m = REGWIDTH_RE.search(line)
            if m and not line.lstrip().startswith("default "):
                regtype_width = parse_int(m.group(1))
            depth += opens - closes
            if depth <= regtype_open_depth:
                reg_types[cur_regtype] = RegTypeDef(
                    name=cur_regtype, regwidth=regtype_width)
                cur_regtype = None
            continue

        # --- Inside an addrmap body ---
        new_depth = depth + opens - closes

        if reg_open_depth is None and REG_OPEN_RE.match(line):
            # Start of an anonymous reg body inside the addrmap.
            reg_open_depth = depth
            depth = new_depth
            if depth <= reg_open_depth:
                m = REG_CLOSE_RE.search(line)
                if m:
                    off = parse_int(m.group(2)) if m.group(2) else None
                    cur_regs.append(RegDef(name=m.group(1), offset=off))
                reg_open_depth = None
            continue

        if reg_open_depth is not None:
            depth = new_depth
            if depth <= reg_open_depth:
                m = REG_CLOSE_RE.search(line)
                if m:
                    off = parse_int(m.group(2)) if m.group(2) else None
                    cur_regs.append(RegDef(name=m.group(1), offset=off))
                reg_open_depth = None
            continue

        m = ALIGN_RE.match(line)
        if m:
            cur_align = parse_int(m.group(1))
            depth = new_depth
            continue

        m = DEFAULT_REGWIDTH_RE.match(line)
        if m:
            cur_regwidth = parse_int(m.group(1))
            depth = new_depth
            continue

        placement = parse_placement_line(line)
        if placement is not None:
            cur_placements.append(placement)

        depth = new_depth
        if depth <= addrmap_open_depth:
            addrmaps[cur_name] = AddrmapDef(
                name=cur_name,
                placements=cur_placements,
                regs=cur_regs,
                alignment=cur_align,
                regwidth=cur_regwidth,
            )
            cur_name = None
            addrmap_open_depth = -1
            reg_open_depth = None

    return addrmaps, reg_types


def compute_reg_block_size(addrmap: AddrmapDef) -> int:
    """Derive an addrmap's block size from its reg definitions.
      - regs with explicit `@ offset` sit at that byte offset
      - sequential regs occupy stride = max(alignment, regwidth/8) each
    Returns the max(offset + reg_size) across all regs, or 0 if none."""
    reg_bytes = addrmap.regwidth // 8 if addrmap.regwidth else 4
    stride = max(addrmap.alignment or 1, reg_bytes)
    cursor = 0
    max_extent = 0
    for r in addrmap.regs:
        off = r.offset if r.offset is not None else cursor
        cursor = off + stride
        max_extent = max(max_extent, off + reg_bytes)
    return max_extent


# Back-compat shim: some callers only need the placements dict.
def parse_addrmaps(path: Path) -> dict[str, list[Placement]]:
    addrmaps, _ = parse_addrmap_defs(path)
    return {name: d.placements for name, d in addrmaps.items()}


def parse_memdefs(path: Path) -> dict[str, MemDef]:
    memdefs: dict[str, MemDef] = {}
    mem_name: str | None = None
    brace_depth = 0
    in_mem = False
    mementries: int | None = None
    memwidth: int | None = None

    for line in sanitize_lines(path):
        if not line:
            continue

        if not in_mem:
            m = MEM_RE.match(line)
            if m:
                mem_name = m.group(1)
                in_mem = True
                brace_depth = line.count("{") - line.count("}")
                mementries = None
                memwidth = None
            continue

        mentries_match = MENTRIES_RE.search(line)
        if mentries_match:
            mementries = parse_int(mentries_match.group(1))
        memwidth_match = MEMWIDTH_RE.search(line)
        if memwidth_match:
            memwidth = parse_int(memwidth_match.group(1))

        brace_depth += line.count("{") - line.count("}")
        if brace_depth > 0:
            continue

        if mem_name is not None and mementries is not None and memwidth is not None:
            if memwidth % 8 != 0:
                sys.exit(f"[top2h] memwidth for {mem_name} is not byte-aligned: {memwidth}")
            memdefs[mem_name] = MemDef(
                name=mem_name,
                size=mementries * (memwidth // 8),
            )

        mem_name = None
        in_mem = False

    return memdefs


def parse_top_rdl(path: Path):
    all_addrmaps: dict[str, AddrmapDef] = {}
    all_memdefs: dict[str, MemDef] = {}
    all_reg_types: dict[str, RegTypeDef] = {}
    for file_path in parse_available_includes(path):
        addrmaps, reg_types = parse_addrmap_defs(file_path)
        all_addrmaps.update(addrmaps)
        all_reg_types.update(reg_types)
        all_memdefs.update(parse_memdefs(file_path))

    top_defs, _ = parse_addrmap_defs(path)
    if not top_defs:
        sys.exit(f"[top2h] no addrmap found in {path}")
    top_name = next(iter(top_defs))
    top_def = all_addrmaps.get(top_name)
    if top_def is None or not top_def.placements:
        sys.exit(f"[top2h] no top-level placements found in {path}")
    return top_name, top_def.placements, all_addrmaps, all_memdefs, all_reg_types


def definition_name(placement: Placement) -> str:
    if placement.kind.startswith("external "):
        return placement.export_name
    return placement.kind


def resolve_size(kind: str, addrmaps: dict[str, AddrmapDef],
                 memdefs: dict[str, MemDef],
                 reg_types: dict[str, RegTypeDef],
                 size_cache: dict[str, int | None]) -> int | None:
    """Return the byte size of an addrmap / mem / named reg-type kind.

    Resolution order:
      1. mem blocks: mementries * memwidth/8 (explicit)
      2. named reg types (reg <Name> { ... };): regwidth/8 bytes
      3. addrmaps with sub-placements: max(address + resolved child_size)
      4. addrmaps with anonymous reg children: highest occupied offset + size
         (implicit from alignment + explicit or sequential @ offsets)
      5. None — nothing we can derive."""
    if kind in size_cache:
        return size_cache[kind]

    memdef = memdefs.get(kind)
    if memdef is not None:
        size_cache[kind] = memdef.size
        return memdef.size

    regtype = reg_types.get(kind)
    if regtype is not None:
        size_cache[kind] = regtype.regwidth // 8
        return size_cache[kind]

    addrmap = addrmaps.get(kind)
    if addrmap is None:
        size_cache[kind] = None
        return None

    max_extent = 0
    found_child = False

    for placement in addrmap.placements:
        child_size = resolve_size(
            definition_name(placement), addrmaps, memdefs, reg_types, size_cache
        )
        if child_size is None:
            continue
        found_child = True
        max_extent = max(max_extent, placement.address + child_size)

    reg_extent = compute_reg_block_size(addrmap)
    if reg_extent > 0:
        found_child = True
        max_extent = max(max_extent, reg_extent)

    size_cache[kind] = max_extent if found_child else None
    return size_cache[kind]


def emit_placement(out: list[str], macro_prefix: str, placement: Placement,
                   absolute_address: int, size: int | None,
                   seen_macros: dict[str, int]) -> None:
    macro = f"{macro_prefix}_{placement.export_name.upper()}_BASE"
    if macro in seen_macros:
        if seen_macros[macro] != absolute_address:
            sys.exit(
                f"[top2h] conflicting values for {macro}: "
                f"0x{seen_macros[macro]:X} vs 0x{absolute_address:X}"
            )
        return

    seen_macros[macro] = absolute_address
    out.append(f"/* Instance: {placement.kind} {placement.instance} */")
    out.append(f"#define {macro} {hex_literal(absolute_address)}")
    if size is not None:
        out.append(
            f"#define {macro_prefix}_{placement.export_name.upper()}_SIZE "
            f"{hex_literal(size)}"
        )
        out.append(
            f"#define {macro_prefix}_{placement.export_name.upper()}_END "
            f"{hex_literal(absolute_address + size)}"
        )
    out.append("")


def emit_nested_placements(out: list[str], macro_prefix: str,
                           placement: Placement, absolute_address: int,
                           addrmaps: dict[str, AddrmapDef],
                           memdefs: dict[str, MemDef],
                           reg_types: dict[str, RegTypeDef],
                           size_cache: dict[str, int | None],
                           seen_macros: dict[str, int]) -> None:
    parent = addrmaps.get(placement.kind)
    if parent is None:
        return
    for child in parent.placements:
        if not child.kind.startswith("external "):
            continue
        child_abs = absolute_address + child.address
        child_size = resolve_size(
            definition_name(child), addrmaps, memdefs, reg_types, size_cache
        )
        emit_placement(out, macro_prefix, child, child_abs, child_size, seen_macros)


def emit_header(input_path: Path, output_path: Path, prefix: str | None) -> str:
    top_name, entries, addrmaps, memdefs, reg_types = parse_top_rdl(input_path)
    macro_prefix = prefix or top_name.upper()
    guard = f"_{output_path.name.replace('.', '_').upper()}_"
    seen_macros: dict[str, int] = {}
    size_cache: dict[str, int | None] = {}

    out = []
    out.append(f"/* Auto-generated by top2h.py from {input_path.name}. Do not edit. */")
    out.append(f"#ifndef {guard}")
    out.append(f"#define {guard}")
    out.append("")
    out.append("/* ============================================================ */")
    out.append(f"/* Top map: {top_name} */")
    out.append("/* ============================================================ */")
    out.append("")

    for placement in entries:
        placement_size = resolve_size(
            definition_name(placement), addrmaps, memdefs, reg_types, size_cache
        )
        emit_placement(
            out, macro_prefix, placement, placement.address, placement_size, seen_macros
        )
        emit_nested_placements(
            out, macro_prefix, placement, placement.address, addrmaps, memdefs,
            reg_types, size_cache, seen_macros
        )

    out.append(f"#endif /* {guard} */")
    out.append("")
    return "\n".join(out)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("input", type=Path, help="Top-level RDL file to parse.")
    ap.add_argument("-o", "--out", type=Path, required=True,
                    help="Output header path.")
    ap.add_argument("-p", "--prefix", default=None,
                    help="Macro prefix override (default: top addrmap name).")
    args = ap.parse_args()

    text = emit_header(args.input.resolve(), args.out.resolve(), args.prefix)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
