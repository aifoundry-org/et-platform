#!/usr/bin/env python3
"""Generate a Semifore-style C header from a SystemRDL source.

Emits per-register and per-field macros compatible with ET-SoC1's csrCompile
output style, plus block-level typedef struct for struct-cast access:

  Per-register:
    <BLOCK>_<REG>_ADDRESS       byte offset
    <BLOCK>_<REG>_BYTE_ADDRESS  (alias for ADDRESS)
    <BLOCK>_<REG>_OFFSET        same as ADDRESS (for compat)
    <BLOCK>_<REG>_BYTE_OFFSET   same as BYTE_ADDRESS
    <BLOCK>_<REG>_SIZE          size in bytes
    <BLOCK>_<REG>_BYTE_SIZE     (alias for SIZE)
    <BLOCK>_<REG>_READ_ACCESS   0 or 1
    <BLOCK>_<REG>_WRITE_ACCESS  0 or 1
    <BLOCK>_<REG>_RESET_VALUE   combined reset value
    <BLOCK>_<REG>_RESET_MASK    bits with defined reset
    <BLOCK>_<REG>_READ_MASK     readable bits
    <BLOCK>_<REG>_WRITE_MASK    writable bits

  Per-field:
    <BLOCK>_<REG>_<FIELD>_MSB
    <BLOCK>_<REG>_<FIELD>_LSB
    <BLOCK>_<REG>_<FIELD>_WIDTH
    <BLOCK>_<REG>_<FIELD>_READ_ACCESS
    <BLOCK>_<REG>_<FIELD>_WRITE_ACCESS
    <BLOCK>_<REG>_<FIELD>_RESET
    <BLOCK>_<REG>_<FIELD>_FIELD_MASK
    <BLOCK>_<REG>_<FIELD>_GET(x)
    <BLOCK>_<REG>_<FIELD>_SET(x)
    <BLOCK>_<REG>_<FIELD>_MODIFY(r, x)

  Per-enum (when SystemRDL 'encode' is used):
    <BLOCK>_<REG>_<FIELD>_<ENUM_NAME>  value

  Block-level typedef:
    typedef struct {
        volatile uint32_t REG0;   /* Offset 0x0  */
        uint8_t _pad0[0x4];       /* 0x4..0x7 reserved */
        volatile uint32_t REG1;   /* Offset 0x8  */
        ...
    } <Block>_t, *PTR_<Block>_t;

  Register-aliasing union (when multiple regs share the same offset):
    typedef union {
        volatile uint32_t RBR;
        volatile uint32_t DLL;
        volatile uint32_t THR;
    } <Block>_<UnionName>, *PTR_<Block>_<UnionName>;

Usage:
    rdl2hal.py input.rdl -o output.h [-t TOP] [-I INCDIR ...]
"""
import argparse
import sys
import os
from collections import OrderedDict, defaultdict
from systemrdl import RDLCompiler
from systemrdl.node import AddrmapNode, RegNode, FieldNode
from systemrdl.rdltypes import AccessType


# --- helpers ---------------------------------------------------------------

BLOCK_SUFFIXES = ("_Reg", "_Regs", "_REG", "_REGS")


def normalize_block(name: str) -> str:
    """Strip common suffixes from addrmap names so UART_Reg -> UART."""
    for suffix in BLOCK_SUFFIXES:
        if name.endswith(suffix):
            return name[: -len(suffix)]
    return name


def sw_flags(sw):
    if sw == AccessType.rw:  return 1, 1
    if sw == AccessType.r:   return 1, 0
    if sw == AccessType.w:   return 0, 1
    if sw == AccessType.rw1: return 1, 1
    if sw == AccessType.w1:  return 0, 1
    return 0, 0


def uname(*parts):
    return "_".join(p.upper() for p in parts)


def hex_literal(value, regwidth=32):
    if regwidth <= 32:
        return f"0x{value:08X}ul"
    return f"0x{value:016X}ull"


def ctype_for_width(regwidth: int) -> str:
    return {8: "uint8_t", 16: "uint16_t", 32: "uint32_t", 64: "uint64_t"}.get(
        regwidth, "uint32_t"
    )


def type_name_for_block(block_name: str) -> str:
    """PascalCase-ish type name: 'UART' -> 'Uart_t', 'MINION_CSR' -> 'Minion_csr_t',
    'minion_csr' -> 'Minion_csr_t'."""
    n = normalize_block(block_name)
    if not n:
        return "_t"
    if n.isupper():
        return n[0] + n[1:].lower() + "_t"
    return n[0].upper() + n[1:] + "_t"


# --- emitters --------------------------------------------------------------

def emit_field(out, prefix, f: FieldNode, regwidth):
    fp = uname(prefix, f.inst_name)
    r, w = sw_flags(f.get_property("sw"))
    fmask = ((1 << f.width) - 1) << f.low
    inv_mask = ((1 << regwidth) - 1) ^ fmask
    reset = f.get_property("reset") or 0

    out.append(f"/* Field: {prefix}.{f.inst_name} */")
    out.append(f"#define {fp}_MSB           {f.high}u")
    out.append(f"#define {fp}_LSB           {f.low}u")
    out.append(f"#define {fp}_WIDTH         {f.width}u")
    out.append(f"#define {fp}_READ_ACCESS   {r}u")
    out.append(f"#define {fp}_WRITE_ACCESS  {w}u")
    out.append(f"#define {fp}_RESET         {hex_literal(reset, regwidth)}")
    out.append(f"#define {fp}_FIELD_MASK    {hex_literal(fmask, regwidth)}")

    lsb = f.low
    if lsb == 0:
        out.append(f"#define {fp}_GET(x)        ((x) & {hex_literal(fmask, regwidth)})")
        out.append(f"#define {fp}_SET(x)        ((x) & {hex_literal(fmask, regwidth)})")
    else:
        out.append(
            f"#define {fp}_GET(x)        (((x) & {hex_literal(fmask, regwidth)}) >> {lsb})"
        )
        out.append(
            f"#define {fp}_SET(x)        (((x) << {lsb}) & {hex_literal(fmask, regwidth)})"
        )
    out.append(
        f"#define {fp}_MODIFY(r, x)   "
        f"((((x) << {lsb}) & {hex_literal(fmask, regwidth)}) | "
        f"((r) & {hex_literal(inv_mask, regwidth)}))"
    )

    enc = f.get_property("encode")
    if enc is not None:
        try:
            members = list(enc)
        except TypeError:
            members = []
        for member in members:
            out.append(
                f"#define {fp}_{member.name.upper()}  {hex_literal(member.value, regwidth)}"
            )
    out.append("")


def emit_register(out, block_prefix, reg: RegNode):
    rname = reg.inst_name
    rp = uname(block_prefix, rname)
    regwidth = reg.get_property("regwidth") or 32
    size_bytes = regwidth // 8

    reset_val = 0
    reg_r = reg_w = 0
    reset_mask = read_mask = write_mask = 0
    for f in reg.fields():
        fr = f.get_property("reset") or 0
        fmask = ((1 << f.width) - 1) << f.low
        reset_val |= (fr & ((1 << f.width) - 1)) << f.low
        r, w = sw_flags(f.get_property("sw"))
        reg_r |= r
        reg_w |= w
        reset_mask |= fmask
        if r:
            read_mask |= fmask
        if w:
            write_mask |= fmask

    out.append(f"/* Register: {block_prefix}.{rname} */")
    out.append(f"#define {rp}_ADDRESS        {hex_literal(reg.address_offset, 32)}")
    out.append(f"#define {rp}_BYTE_ADDRESS   {hex_literal(reg.address_offset, 32)}")
    out.append(f"#define {rp}_OFFSET         {hex_literal(reg.address_offset, 32)}")
    out.append(f"#define {rp}_BYTE_OFFSET    {hex_literal(reg.address_offset, 32)}")
    out.append(f"#define {rp}_SIZE           {hex_literal(size_bytes, 32)}")
    out.append(f"#define {rp}_BYTE_SIZE      {hex_literal(size_bytes, 32)}")
    out.append(f"#define {rp}_READ_ACCESS    {reg_r}u")
    out.append(f"#define {rp}_WRITE_ACCESS   {reg_w}u")
    out.append(f"#define {rp}_RESET_VALUE    {hex_literal(reset_val, regwidth)}")
    out.append(f"#define {rp}_RESET_MASK     {hex_literal(reset_mask, regwidth)}")
    out.append(f"#define {rp}_READ_MASK      {hex_literal(read_mask, regwidth)}")
    out.append(f"#define {rp}_WRITE_MASK     {hex_literal(write_mask, regwidth)}")
    out.append("")

    for f in reg.fields():
        emit_field(out, uname(block_prefix, rname), f, regwidth)


def group_by_offset(regs):
    """Return OrderedDict: offset -> list of RegNode sharing that offset."""
    groups = OrderedDict()
    for r in sorted(regs, key=lambda r: r.address_offset):
        groups.setdefault(r.address_offset, []).append(r)
    return groups


def emit_aliasing_unions(out, block_prefix, type_prefix, groups):
    """For each offset with ≥2 registers, emit a union typedef.
    Returns dict: offset -> union type name (or None if no union)."""
    union_names = {}
    for off, regs_at_off in groups.items():
        if len(regs_at_off) < 2:
            union_names[off] = None
            continue
        # Name the union by the first register at that offset
        union_tag = regs_at_off[0].inst_name
        union_type = f"{type_prefix}_{union_tag}"
        out.append(f"/* Union: {block_prefix}.{union_tag} (aliased at offset 0x{off:X}) */")
        out.append("typedef union {")
        for r in regs_at_off:
            rw = r.get_property("regwidth") or 32
            ct = ctype_for_width(rw)
            # Determine access: if any field is writable it's R/W, else R
            has_w = any(sw_flags(f.get_property("sw"))[1] for f in r.fields())
            access = "R/W" if has_w else "R"
            out.append(f"    volatile {ct} {r.inst_name};  /* Offset 0x{off:X} ({access}) */")
        out.append(f"}} {union_type}, *PTR_{union_type};")
        out.append("")
        union_names[off] = union_type
    return union_names


def emit_block_struct(out, block_prefix, type_name, groups, union_names):
    """Emit block-level typedef struct with padding between non-contiguous regs."""
    out.append(f"/* Block-level typedef: {block_prefix} */")
    out.append("typedef struct {")
    cursor = 0
    pad_n = 0
    for off, regs_at_off in groups.items():
        if off > cursor:
            gap = off - cursor
            out.append(f"    uint8_t _pad{pad_n}[0x{gap:X}];  /* 0x{cursor:X}..0x{off-1:X} reserved */")
            pad_n += 1
        if len(regs_at_off) >= 2 and union_names.get(off):
            # Use the union type
            union_type = union_names[off]
            member_name = regs_at_off[0].inst_name
            out.append(f"    {union_type} {member_name};  /* Offset 0x{off:X} */")
            rw = regs_at_off[0].get_property("regwidth") or 32
            cursor = off + (rw // 8)
        else:
            r = regs_at_off[0]
            rw = r.get_property("regwidth") or 32
            ct = ctype_for_width(rw)
            has_w = any(sw_flags(f.get_property("sw"))[1] for f in r.fields())
            access = "R/W" if has_w else "R"
            out.append(f"    volatile {ct} {r.inst_name};  /* Offset 0x{off:X} ({access}) */")
            cursor = off + (rw // 8)
    out.append(f"}} {type_name}, *PTR_{type_name};")
    out.append("")


def emit_block(out, block: AddrmapNode):
    raw_name = block.inst_name
    block_prefix = uname(normalize_block(raw_name))
    type_name = type_name_for_block(raw_name)

    out.append("/* ============================================================ */")
    out.append(f"/* Block: {raw_name}  (macro prefix: {block_prefix}) */")
    out.append("/* ============================================================ */")
    out.append("")

    regs = [r for r in block.descendants(unroll=True) if isinstance(r, RegNode)]
    if not regs:
        return

    # Per-register and per-field macros
    for reg in sorted(regs, key=lambda r: r.address_offset):
        emit_register(out, block_prefix, reg)

    # Group by offset (for aliasing detection)
    groups = group_by_offset(regs)

    # Emit aliasing unions (if any)
    union_names = emit_aliasing_unions(out, block_prefix, normalize_block(raw_name), groups)

    # Emit block-level struct
    emit_block_struct(out, block_prefix, type_name, groups, union_names)


def emit_header(top: AddrmapNode, guard: str):
    out = []
    out.append("/* Auto-generated by rdl2hal.py from SystemRDL. Do not edit. */")
    out.append(f"#ifndef {guard}")
    out.append(f"#define {guard}")
    out.append("")
    out.append("#include <stdint.h>")
    out.append("")
    out.append("#ifdef __cplusplus")
    out.append('extern "C" {')
    out.append("#endif")
    out.append("")

    children = list(top.children(unroll=True))
    sub_addrmaps = [c for c in children if isinstance(c, AddrmapNode)]
    if sub_addrmaps:
        for b in sub_addrmaps:
            emit_block(out, b)
    elif any(isinstance(c, RegNode) for c in children):
        emit_block(out, top)
    else:
        sys.exit("No registers or sub-addrmaps found in top addrmap")

    out.append("#ifdef __cplusplus")
    out.append("}")
    out.append("#endif")
    out.append("")
    out.append(f"#endif /* {guard} */")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser(description="Generate Semifore-style C header from SystemRDL")
    ap.add_argument("rdl_file", help="Input .rdl file")
    ap.add_argument("-o", "--output", required=True, help="Output .h file")
    ap.add_argument("-t", "--top", default=None, help="Top-level addrmap name")
    ap.add_argument("-I", "--incdir", action="append", default=[], help="Include search dirs")
    ap.add_argument("-g", "--guard", default=None, help="Include guard name (default derived from output)")
    args = ap.parse_args()

    rdlc = RDLCompiler()
    rdlc.compile_file(args.rdl_file, incl_search_paths=args.incdir)
    root = rdlc.elaborate(top_def_name=args.top) if args.top else rdlc.elaborate()
    top = root.find_by_path(root.top.inst_name)

    guard = args.guard
    if guard is None:
        base = os.path.basename(args.output)
        guard = "_" + base.replace(".", "_").upper() + "_"

    header = emit_header(top, guard)

    outdir = os.path.dirname(args.output)
    if outdir and not os.path.exists(outdir):
        os.makedirs(outdir, exist_ok=True)
    with open(args.output, "w") as f:
        f.write(header + "\n")

    print(f"Wrote {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
