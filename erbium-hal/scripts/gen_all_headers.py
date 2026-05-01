#!/usr/bin/env python3
"""Regenerate erbium-hal/include/hwinc/ headers from RDL sources.

Pipeline:

    CSV (+ types/*.csv)  ─► csr2rdl.py  ─► minion_csr.rdl  ─┐
                                                            │
                                                            ├─► rdl2hal.py ─► hwinc/*.h
    hand-written RDLs   ────────────────────────────────────┘
    (uart, system, qspi, i2c, plic, esr)

    top_cpu_mm.rdl      ─────────────────────────────────────► top2h.py  ─► hwinc/top.h

This script has NO built-in knowledge of the RDL tree layout. Every
path to an RDL file (and to the csr2rdl.py helper) must be supplied
explicitly, either through CLI flags or environment variables:

  Slot               Env var                         CLI flag
  --------------------------------------------------------------------
  uart               ERBIUM_HAL_UART_RDL             --uart-rdl
  system             ERBIUM_HAL_SYSTEM_RDL           --system-rdl
  qspi               ERBIUM_HAL_QSPI_RDL             --qspi-rdl
  i2c                ERBIUM_HAL_I2C_RDL              --i2c-rdl
  esr                ERBIUM_HAL_ESR_RDL              --esr-rdl
  plic               ERBIUM_HAL_PLIC_RDL             --plic-rdl
  minion_csr         ERBIUM_HAL_MINION_CSR_RDL       --minion-csr-rdl
  top                ERBIUM_HAL_TOP_RDL              --top-rdl
  csr2rdl script     ERBIUM_HAL_CSR2RDL_SCRIPT       --csr2rdl-script

Typical usage (paths exported from a local, un-committed shell script):

    source ~/my-rtl-env.sh
    gen_all_headers.py

Or explicit:

    gen_all_headers.py \\
        --uart-rdl       $UART_RDL \\
        --system-rdl     $SYSTEM_RDL \\
        --qspi-rdl       $QSPI_RDL \\
        --i2c-rdl        $I2C_RDL \\
        --esr-rdl        $ESR_RDL \\
        --plic-rdl       $PLIC_RDL \\
        --minion-csr-rdl $MINION_CSR_RDL \\
        --top-rdl        $TOP_RDL \\
        --csr2rdl-script $CSR2RDL

Use --skip-csr-regen when you already have a committed minion_csr.rdl
and don't want to re-run the CSV→RDL step (in which case csr2rdl-script
is not required).
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_OUT = (SCRIPT_DIR.parent / "include" / "hwinc").resolve()

# Sibling helper scripts shipped in this repo. These are the only paths
# the script knows about — they live relative to itself, not to any
# hardware repo.
RDL2HAL  = SCRIPT_DIR / "rdl2hal.py"
TOP2H    = SCRIPT_DIR / "top2h.py"
CSR2RDL  = SCRIPT_DIR / "csr2rdl.py"

TOP_PREFIX = "ERBIUM_TOP"

# (slot_name, output_header_name, optional_top_addrmap_hint_for_rdl2hal)
#
# The addrmap hint is a string identifier that rdl2hal.py uses to pick
# the top-level addrmap inside the RDL when there is more than one.
# It is not a path and does not leak filesystem layout.
SOURCES = [
    ("uart",        "uart.h",        None),
    ("system",      "system.h",      None),
    ("qspi",        "qspi.h",        None),
    ("i2c",         "i2c.h",         "I2C_Reg"),
    ("esr",         "esr.h",         "Erbium_ESR"),
    ("plic",        "plic.h",        "PLIC_cpu"),
    ("minion_csr",  "minion_csr.h",  None),
]


def env_var_for(slot: str) -> str:
    return f"ERBIUM_HAL_{slot.upper()}_RDL"


def fatal(msg: str) -> None:
    sys.exit(f"[gen_all_headers] {msg}")


def run(cmd: list[str]) -> None:
    printable = " ".join(str(c) for c in cmd)
    print(f"  $ {printable}")
    result = subprocess.run(cmd, check=False)
    if result.returncode != 0:
        fatal(f"command failed: {printable}")


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


def regen_minion_csr_rdl(csr2rdl: Path, minion_csr_rdl_out: Path) -> None:
    if not csr2rdl.exists():
        fatal(f"csr2rdl.py not found at {csr2rdl}")
    print("[1/3] CSV → RDL  (minion_csr via csr2rdl.py)")
    run([
        "python3", str(csr2rdl),
        "--rdl-only",
        "--rdl-out", str(minion_csr_rdl_out),
    ])


def regen_headers(rdl_paths: dict[str, Path], out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f"[2/3] RDL → C headers  (→ {out_dir})")
    for slot, out_name, top_addrmap in SOURCES:
        rdl = rdl_paths[slot]
        if not rdl.exists():
            fatal(f"missing RDL source for '{slot}': {rdl}")
        cmd = [
            "python3", str(RDL2HAL),
            str(rdl),
            "-o", str(out_dir / out_name),
        ]
        if top_addrmap:
            cmd += ["-t", top_addrmap]
        run(cmd)


def regen_top_header(top_rdl: Path, out_dir: Path) -> None:
    if not top_rdl.exists():
        fatal(f"missing top-level RDL source: {top_rdl}")
    print(f"[3/3] Top map → C header  (→ {out_dir / 'top.h'})")
    run([
        "python3", str(TOP2H),
        str(top_rdl),
        "-o", str(out_dir / "top.h"),
        "-p", TOP_PREFIX,
    ])


def main() -> None:
    ap = argparse.ArgumentParser(
        description=__doc__.splitlines()[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    for slot, _out, _top in SOURCES:
        ap.add_argument(
            f"--{slot.replace('_', '-')}-rdl",
            dest=f"{slot}_rdl", type=Path, default=None,
            help=f"Path to the {slot} RDL source. "
                 f"Defaults to ${env_var_for(slot)}.",
        )
    ap.add_argument(
        "--top-rdl", dest="top_rdl", type=Path, default=None,
        help="Path to the top-level address-map RDL. "
             "Defaults to $ERBIUM_HAL_TOP_RDL.",
    )
    ap.add_argument(
        "--csr2rdl-script", dest="csr2rdl_script", type=Path, default=None,
        help=f"Path to csr2rdl.py (produces minion_csr.rdl from CSVs). "
             f"Defaults to $ERBIUM_HAL_CSR2RDL_SCRIPT, or the sibling "
             f"copy at {CSR2RDL}. Not required when --skip-csr-regen.",
    )
    ap.add_argument(
        "--out", type=Path, default=DEFAULT_OUT,
        help=f"Output directory for generated headers "
             f"(default: {DEFAULT_OUT}).",
    )
    ap.add_argument(
        "--skip-csr-regen", action="store_true",
        help="Skip the CSV→RDL step and use the minion_csr RDL as-is.",
    )
    args = ap.parse_args()

    if not RDL2HAL.exists():
        fatal(f"rdl2hal.py not found at {RDL2HAL}")
    if not TOP2H.exists():
        fatal(f"top2h.py not found at {TOP2H}")

    # Resolve every input path from CLI or env.
    rdl_paths: dict[str, Path] = {
        slot: resolve_path(
            getattr(args, f"{slot}_rdl"), env_var_for(slot), f"{slot}_rdl"
        )
        for slot, _, _ in SOURCES
    }
    top_rdl = resolve_path(args.top_rdl, "ERBIUM_HAL_TOP_RDL", "top_rdl")
    out_dir = args.out.expanduser().resolve()

    if not args.skip_csr_regen:
        # Prefer CLI flag, then env var, then the sibling shipped copy.
        csr2rdl = (
            (args.csr2rdl_script.expanduser().resolve()
             if args.csr2rdl_script else None)
            or (Path(os.environ["ERBIUM_HAL_CSR2RDL_SCRIPT"]).expanduser().resolve()
                if os.environ.get("ERBIUM_HAL_CSR2RDL_SCRIPT") else None)
            or CSR2RDL
        )
        regen_minion_csr_rdl(csr2rdl, rdl_paths["minion_csr"])
    else:
        print("[skip] CSV → RDL (using existing minion_csr.rdl as-is)")
        if not rdl_paths["minion_csr"].exists():
            fatal(
                f"minion_csr.rdl at {rdl_paths['minion_csr']} does not exist — "
                f"cannot --skip-csr-regen"
            )

    regen_headers(rdl_paths, out_dir)
    regen_top_header(top_rdl, out_dir)
    print(f"\nWrote {len(SOURCES) + 1} headers to {out_dir}")


if __name__ == "__main__":
    main()
