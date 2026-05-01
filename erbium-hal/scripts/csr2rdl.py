#!/usr/bin/env python3
"""Convert Minion CSR CSV sources into synthetic SystemRDL.

Builds a byte-addressed synthetic CSR bank:

    CSR byte offset = csr_number * 8

Inputs (all paths must be provided — no built-in defaults, to avoid
hard-coding any particular hardware-repo layout):

  Input              Env var                          CLI flag
  --------------------------------------------------------------------
  semifore CSV       ERBIUM_HAL_CSR_SEMIFORE_CSV      --semifore-csv
  register CSV       ERBIUM_HAL_CSR_REG_CSV           --reg-csv
  types dir          ERBIUM_HAL_CSR_TYPES_DIR         --types-dir
  RDL output         ERBIUM_HAL_CSR_RDL_OUT           --rdl-out

The --rdl-only mode (the only mode used by gen_all_headers.py)
generates the RDL file and exits. The optional c-header pipeline
(peakrdl + rdl2h.py) is kept for upstream compatibility but also
requires all its paths to be supplied explicitly.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


RESERVED_NAMES = {"WIRI", "WPRI", "ZERO"}


@dataclass
class FieldInfo:
    name: str
    low: int
    high: int
    access: str
    reset: int = 0
    desc: str = ""
    onwrite: str | None = None
    onread: str | None = None

    @property
    def width(self) -> int:
        return self.high - self.low + 1


@dataclass
class RegisterInfo:
    name: str
    csr_number: int
    access: str
    title: str = ""
    desc: str = ""
    reset: int | None = None
    reset_mask: int | None = None
    type_name: str | None = None
    fields: list[FieldInfo] = field(default_factory=list)

    @property
    def byte_offset(self) -> int:
        return self.csr_number * 8


def fatal(msg: str) -> None:
    sys.exit(f"[csr2rdl] {msg}")


def resolve_path(cli_value: Path | None, env_name: str, label: str) -> Path:
    """Resolve a path from CLI arg or env var, or abort with a clear error."""
    if cli_value is not None:
        return cli_value.expanduser().resolve()
    env_value = os.environ.get(env_name)
    if env_value:
        return Path(env_value).expanduser().resolve()
    fatal(
        f"{label} path not provided.\n"
        f"  Set ${env_name} or pass --{label.replace('_', '-')}"
    )


def parse_num(text: str | None) -> int | None:
    if text is None:
        return None
    value = text.strip().strip('"')
    if not value:
        return None
    if "'" in value:
        _width, digits = value.split("'", 1)
        base = digits[0].lower()
        number = digits[1:].replace("_", "")
        if base == "h":
            return int(number, 16)
        if base == "d":
            return int(number, 10)
        if base == "b":
            return int(number, 2)
        if base == "o":
            return int(number, 8)
        raise ValueError(f"unsupported literal: {text}")
    return int(value, 0)


def sanitize_identifier(name: str) -> str:
    identifier = re.sub(r"[^0-9A-Za-z_]+", "_", name.strip())
    identifier = re.sub(r"_+", "_", identifier).strip("_")
    if not identifier:
        identifier = "unnamed"
    if identifier[0].isdigit():
        identifier = f"csr_{identifier}"
    return identifier.lower()


def clean_text(text: str | None) -> str:
    if not text:
        return ""
    text = text.replace("\r", " ").replace("\n", " ")
    text = re.sub(r"\s+", " ", text).strip()
    text = text.replace("\\", "\\\\").replace('"', '\\"')
    return text


def parse_position(text: str) -> tuple[int, int]:
    value = text.strip()
    match = re.fullmatch(r"\[(\d+):(\d+)\]", value)
    if match:
        high = int(match.group(1))
        low = int(match.group(2))
        if high < low:
            high, low = low, high
        return low, high
    match = re.fullmatch(r"\[(\d+)\]", value)
    if match:
        bit = int(match.group(1))
        return bit, bit
    raise ValueError(f"unsupported bit position: {text}")


def map_sw(access: str) -> str:
    value = access.upper().replace(" ", "")
    if value in {"R", "RO"}:
        return "r"
    if value in {"W", "WO"}:
        return "w"
    return "rw"


def map_onwrite(access: str) -> str | None:
    value = access.upper().replace(" ", "")
    if value.endswith("1C"):
        return "woclr"
    if value.endswith("1S"):
        return "woset"
    if value.endswith("0C"):
        return "wzc"
    if value.endswith("0S"):
        return "wzs"
    if value.endswith("1T"):
        return "wot"
    return None


def is_reserved_type_field(name: str) -> bool:
    upper = sanitize_identifier(name).upper()
    return upper in RESERVED_NAMES or upper.endswith("_ZERO")


def load_type_map(path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    with path.open(newline="", encoding="latin-1") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            address = (row.get("Address") or "").strip()
            name = (row.get("Name") or "").strip()
            type_name = (row.get("Type") or "").strip()
            if address.startswith("0x") and name:
                result[name] = type_name
    return result


def load_type_fields(types_dir: Path, type_name: str, register_reset: int | None) -> list[FieldInfo]:
    path = types_dir / f"{type_name}.csv"
    if not path.exists():
        return []

    rows: dict[str, list[str]] = {}
    with path.open(newline="", encoding="latin-1") as handle:
        reader = csv.reader(handle)
        for row in reader:
            if row and row[0]:
                rows[row[0]] = row[1:]

    names = list(reversed(rows.get("fields", [])))
    widths = list(reversed(rows.get("width", [])))
    ro_flags = list(reversed(rows.get("RO", [])))
    if not names or not widths:
        return []

    while len(ro_flags) < len(names):
        ro_flags.insert(0, "NO")

    fields: list[FieldInfo] = []
    shift = 0
    for raw_name, raw_width, raw_ro in zip(names, widths, ro_flags):
        if not raw_width.strip():
            continue
        width = int(raw_width, 0)
        name = sanitize_identifier(raw_name)
        if not is_reserved_type_field(raw_name):
            mask = (1 << width) - 1
            reset = 0
            if register_reset is not None:
                reset = (register_reset >> shift) & mask
            access = "r" if raw_ro.strip().upper() == "YES" else "rw"
            fields.append(
                FieldInfo(
                    name=name,
                    low=shift,
                    high=shift + width - 1,
                    access=access,
                    reset=reset,
                )
            )
        shift += width
    return fields


def load_semifore_registers(
    path: Path, type_map: dict[str, str], types_dir: Path
) -> list[RegisterInfo]:
    registers: list[RegisterInfo] = []
    current: RegisterInfo | None = None
    with path.open(newline="", encoding="latin-1") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            offset = (row.get("Offset") or "").strip()
            position = (row.get("Position") or "").strip()
            identifier = (row.get("Identifier") or "").strip()
            access = (row.get("Access") or "").strip()
            row_type = (row.get("Type") or "").strip().lower()

            if offset.startswith("0x") and row_type == "register" and identifier:
                current = RegisterInfo(
                    name=sanitize_identifier(identifier),
                    csr_number=int(offset, 0),
                    access=map_sw(access or "R/W"),
                    title=clean_text(row.get("Title")),
                    desc=clean_text(row.get("Description")),
                    reset=parse_num(row.get("Reset Value")),
                    reset_mask=parse_num(row.get("Reset Mask")),
                    type_name=type_map.get(identifier),
                )
                registers.append(current)
                continue

            if current is None or not position or not identifier:
                continue

            low, high = parse_position(position)
            current.fields.append(
                FieldInfo(
                    name=sanitize_identifier(identifier),
                    low=low,
                    high=high,
                    access=map_sw(access or "R/W"),
                    reset=parse_num(row.get("Reset Value")) or 0,
                    desc=clean_text(row.get("Description")),
                    onwrite=map_onwrite(access or ""),
                )
            )

    for register in registers:
        explicit = [field for field in register.fields]
        use_explicit = False
        if explicit:
            if len(explicit) > 1:
                use_explicit = True
            else:
                only = explicit[0]
                if not (only.name == register.name and only.low == 0 and only.high == 63):
                    use_explicit = True

        if not use_explicit:
            if register.type_name:
                register.fields = load_type_fields(types_dir, register.type_name, register.reset)
            else:
                register.fields = explicit

        if not register.fields:
            register.fields = [
                FieldInfo(
                    name=register.name,
                    low=0,
                    high=63,
                    access=register.access,
                    reset=register.reset or 0,
                    desc=register.desc,
                )
            ]

    return registers


def format_reset(value: int) -> str:
    return hex(value)


def render_field(field: FieldInfo) -> list[str]:
    lines = ["        field {"]
    lines.append(f"            sw = {field.access};")
    if field.onwrite:
        lines.append(f"            onwrite = {field.onwrite};")
    if field.onread:
        lines.append(f"            onread = {field.onread};")
    if field.desc:
        lines.append(f'            desc = "{field.desc}";')
    rng = f"[{field.high}:{field.low}]"
    lines.append(f"        }} {field.name}{rng} = {format_reset(field.reset)};")
    return lines


def render_rdl(registers: list[RegisterInfo], top_name: str) -> str:
    lines = [
        "// Generated by csr2rdl.py",
        "// Synthetic byte-addressed CSR bank: byte_offset = csr_number * 8",
        "",
        f"addrmap {top_name} {{",
        f'    name = "{top_name}";',
        "    alignment = 8;",
        "    default regwidth = 64;",
        "",
    ]

    for register in registers:
        lines.append("    reg {")
        if register.title:
            lines.append(f'        name = "{register.title}";')
        if register.desc:
            lines.append(f'        desc = "{register.desc}";')
        for field in sorted(register.fields, key=lambda item: (item.low, item.high)):
            lines.extend(render_field(field))
        lines.append(f"    }} {register.name} @ 0x{register.byte_offset:X};")
        lines.append("")

    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def run_header_pipeline(rdl_path: Path, header_path: Path, helper_path: Path, top_name: str) -> None:
    header_path.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            "peakrdl",
            "c-header",
            str(rdl_path),
            "-b", "ltoh",
            "-i",
            "-o", str(header_path),
        ],
        check=True,
    )
    subprocess.run(
        [
            sys.executable,
            str(helper_path),
            str(rdl_path),
            str(header_path),
            "-t", top_name,
        ],
        check=True,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__.splitlines()[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--semifore-csv", type=Path, default=None,
        help="Semifore CSV (register table). Defaults to $ERBIUM_HAL_CSR_SEMIFORE_CSV.",
    )
    parser.add_argument(
        "--reg-csv", type=Path, default=None,
        help="Register-to-type mapping CSV. Defaults to $ERBIUM_HAL_CSR_REG_CSV.",
    )
    parser.add_argument(
        "--types-dir", type=Path, default=None,
        help="Directory containing types/*.csv. Defaults to $ERBIUM_HAL_CSR_TYPES_DIR.",
    )
    parser.add_argument(
        "--rdl-out", type=Path, default=None,
        help="Output path for the generated RDL. Defaults to $ERBIUM_HAL_CSR_RDL_OUT.",
    )
    parser.add_argument(
        "--header-out", type=Path, default=None,
        help="Optional C-header output path (requires peakrdl and --rdl2h).",
    )
    parser.add_argument(
        "--rdl2h", type=Path, default=None,
        help="Path to helper rdl2h.py used by the optional header pipeline.",
    )
    parser.add_argument("--top-name", default="minion_csr")
    parser.add_argument(
        "--rdl-only", action="store_true",
        help="Write the RDL only; skip the c-header pipeline.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    semifore_csv = resolve_path(
        args.semifore_csv, "ERBIUM_HAL_CSR_SEMIFORE_CSV", "semifore_csv"
    )
    reg_csv = resolve_path(
        args.reg_csv, "ERBIUM_HAL_CSR_REG_CSV", "reg_csv"
    )
    types_dir = resolve_path(
        args.types_dir, "ERBIUM_HAL_CSR_TYPES_DIR", "types_dir"
    )
    rdl_out = resolve_path(
        args.rdl_out, "ERBIUM_HAL_CSR_RDL_OUT", "rdl_out"
    )

    for p, kind in [(semifore_csv, "file"), (reg_csv, "file"), (types_dir, "dir")]:
        if kind == "file" and not p.is_file():
            fatal(f"input file not found: {p}")
        if kind == "dir" and not p.is_dir():
            fatal(f"input directory not found: {p}")

    type_map = load_type_map(reg_csv)
    registers = load_semifore_registers(semifore_csv, type_map, types_dir)
    rdl_text = render_rdl(registers, args.top_name)

    rdl_out.parent.mkdir(parents=True, exist_ok=True)
    rdl_out.write_text(rdl_text)
    print(f"[csr2rdl] wrote {rdl_out}")

    if not args.rdl_only:
        if args.header_out is None or args.rdl2h is None:
            fatal(
                "--header-out and --rdl2h are required when not in --rdl-only mode"
            )
        run_header_pipeline(rdl_out, args.header_out, args.rdl2h, args.top_name)


if __name__ == "__main__":
    main()
