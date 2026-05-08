#!/usr/bin/env python3
# Copyright (c) 2026 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
"""
End-to-end test for the erbium-examples histogram kernel.

Generates (or loads) a 256x256 grayscale image, launches the
erbium_launcher with the histogram.elf, dumps device memory
after the run, parses the on-device histogram + summary, compares
against a host-side reference, and writes all artifacts to a results
directory so headless users can inspect over sshfs.

Example:
    ./test_histogram.py --output-dir /tmp/hist-out

Expected outputs in <output-dir>:
    input.png              generated/used test image (256x256 grayscale)
    input.bin              raw bytes shipped to the device
    dump_after.bin         full 16 MB device memory dump after kernel return
    summary.txt            pass/fail report + parsed summary struct
    histogram.txt          device vs reference histogram, bin-by-bin
    histogram.png          bar chart of device vs reference bins
    launcher.stdout/.err   captured launcher output for debugging
"""

from __future__ import annotations

import argparse
import struct
import subprocess
import sys
from pathlib import Path

import numpy as np

# Headless plotting.
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

# ------------------------------------------------------------------
# Memory contract — must match erbium-examples/src/histogram/histogram.c
# Buffers are carved out of the top of heap0, so offsets are
# expressed from the end of the device buffer (heap0_end, which is
# always region0_base + region0_size regardless of how the runtime
# loader relocated the kernel).
# ------------------------------------------------------------------
IMG_W = 256
IMG_H = 256
BINS = 256
# Kernel runs on even-numbered harts only (one hart per minion) to
# rule out thread-0 / thread-1 L1 sharing. Must match NUM_HARTS in
# erbium-examples/src/histogram/histogram.c.
NUM_HARTS = 8
ROWS_PER_HART = IMG_H // NUM_HARTS
STRIPE_BYTES = ROWS_PER_HART * IMG_W
TOTAL_PIXELS = IMG_W * IMG_H

DEVICE_BUFFER_SIZE = 16 * 1024 * 1024  # matches kErbiumMemSize in the launcher

INPUT_BYTES = IMG_W * IMG_H                  # 64 KiB  (64 B aligned)
LOCALS_BYTES = NUM_HARTS * BINS * 4           # 16 KiB  (per-hart scratch, 64 B aligned)
OUTPUT_BYTES = BINS * 4                      #  1 KiB  (64 B aligned)
# Per-hart stripe byte-sum checksums: one 64 B cache line per hart.
# Hart H writes its u32 into csums[H * 16]; the remaining 15 u32
# slots pad the line so writes don't false-share across harts.
CSUMS_BYTES = NUM_HARTS * 64                 #  1 KiB
SUMMARY_BYTES = 64                           # padded to full cache line

LAYOUT_BYTES = (INPUT_BYTES + LOCALS_BYTES + OUTPUT_BYTES
                + CSUMS_BYTES + SUMMARY_BYTES)  # 0x14840

# Layout (top of device buffer):
# heap_end - LAYOUT         .. heap_end - (LOCALS+OUTPUT+CSUMS+SUMMARY)  INPUT
# heap_end - (L+O+C+S)      .. heap_end - (OUTPUT+CSUMS+SUMMARY)          LOCALS
# heap_end - (O+C+S)        .. heap_end - (CSUMS+SUMMARY)                 OUTPUT
# heap_end - (CSUMS+SUMMARY).. heap_end - SUMMARY                         CSUMS
# heap_end - SUMMARY        .. heap_end                                   SUMMARY
IN_OFFSET = DEVICE_BUFFER_SIZE - LAYOUT_BYTES
OUT_OFFSET = DEVICE_BUFFER_SIZE - (OUTPUT_BYTES + CSUMS_BYTES + SUMMARY_BYTES)
CSUMS_OFFSET = DEVICE_BUFFER_SIZE - (CSUMS_BYTES + SUMMARY_BYTES)
SUMMARY_OFFSET = DEVICE_BUFFER_SIZE - SUMMARY_BYTES

MAGIC = 0xE0B10157

# ------------------------------------------------------------------
# Defaults
# ------------------------------------------------------------------
DEFAULT_LAUNCHER = "erbium_run"

# Per-device default ELF paths.  Same histogram.c is built for two
# backends (see gp-sdk/device/tests/histogram/CMakeLists.txt):
# soc1sim + sys_emu run the erbium-soc1sim build (kernel hosted on
# ET-SoC-1 silicon/sysemu); erbium_emu runs the native-erbium build.
DEFAULT_ELFS = {
    "soc1sim":    Path("/opt/et/kernels/histogram.erbium-soc1sim.elf"),
    "sys_emu":    Path("/opt/et/kernels/histogram.erbium-soc1sim.elf"),
    "erbium_emu": Path("/opt/et/kernels/histogram.erbium.elf"),
}

# Classic test images bundled under ../testdata/. Use --preset <name>
# to pick one; see testdata/README.md for provenance. All are
# resized to IMG_W x IMG_H grayscale by load_image().
TESTDATA_DIR = Path(__file__).resolve().parent / "testdata"
PRESETS = {
    "baboon":  TESTDATA_DIR / "baboon.jpg",
    "peppers": TESTDATA_DIR / "peppers.jpg",
    "aero":    TESTDATA_DIR / "aero1.jpg",
}


# ------------------------------------------------------------------
# Image handling
# ------------------------------------------------------------------
def generate_image(seed: int) -> np.ndarray:
    """Deterministic 256x256 grayscale pseudo-random image."""
    rng = np.random.default_rng(seed)
    img = rng.integers(0, 256, size=(IMG_H, IMG_W), dtype=np.uint8)
    return img


def load_image(path: Path) -> np.ndarray:
    """Load any image file via Pillow, force to 256x256 grayscale."""
    from PIL import Image

    im = Image.open(path).convert("L")
    if im.size != (IMG_W, IMG_H):
        im = im.resize((IMG_W, IMG_H))
    return np.asarray(im, dtype=np.uint8)


def save_image_png(img: np.ndarray, path: Path) -> None:
    from PIL import Image

    Image.fromarray(img, mode="L").save(path)


# ------------------------------------------------------------------
# Launcher invocation
# ------------------------------------------------------------------
def run_launcher(
    launcher: str,
    elf: Path,
    image_bin: Path,
    dump_after: Path,
    stdout_path: Path,
    stderr_path: Path,
    device: str = "soc1sim",
    extra: list[str] | None = None,
) -> int:
    cmd = [
        launcher,
        "--device", device,
        "--elf-load",
        str(elf),
        "--file_load",
        f"0x{IN_OFFSET:x},{image_bin}",
        "--dump_after",
        str(dump_after),
        "-v",
    ] + (extra or [])
    print("+ " + " ".join(cmd))
    r = subprocess.run(cmd, capture_output=True, text=True)
    stdout_path.write_text(r.stdout)
    stderr_path.write_text(r.stderr)
    if r.returncode != 0:
        print(f"launcher failed with exit {r.returncode}; see {stderr_path}",
              file=sys.stderr)
    return r.returncode


# ------------------------------------------------------------------
# Dump parsing
# ------------------------------------------------------------------
def parse_dump(dump: Path):
    """Return (histogram ndarray[256] uint32, csums ndarray[NUM_HARTS] uint32, summary dict)."""
    data = dump.read_bytes()
    if len(data) < SUMMARY_OFFSET + 32:
        raise RuntimeError(
            f"dump is only {len(data)} bytes, need at least {SUMMARY_OFFSET + 32}"
        )

    hist = np.frombuffer(data, dtype=np.uint32, count=BINS, offset=OUT_OFFSET).copy()

    # Per-hart checksums: CSUMS region has 16 u32 per hart (64 B line);
    # hart H's checksum is at slot 0 of its line, i.e. u32 index H*16.
    csums_all = np.frombuffer(
        data, dtype=np.uint32, count=NUM_HARTS * 16, offset=CSUMS_OFFSET
    ).copy()
    csums = csums_all[::16].copy()  # strided view, one per hart

    raw = data[SUMMARY_OFFSET : SUMMARY_OFFSET + 32]
    (magic, width, height, total_pixels, sum_of_bins,
     max_count, max_index, reserved) = struct.unpack("<8I", raw)

    summary = {
        "magic": magic,
        "width": width,
        "height": height,
        "total_pixels": total_pixels,
        "sum_of_bins": sum_of_bins,
        "max_count": max_count,
        "max_index": max_index,
        "_reserved": reserved,
    }
    return hist, csums, summary


def reference_stripe_csums(img: np.ndarray) -> np.ndarray:
    """Per-hart stripe byte-sums computed host-side, matching the
    kernel's Phase 0 accumulation pattern (each hart owns
    ROWS_PER_HART consecutive rows, row-major in memory). Result is
    a u32 vector of length NUM_HARTS."""
    flat = img.astype(np.uint32).flatten()
    out = np.empty(NUM_HARTS, dtype=np.uint32)
    for h in range(NUM_HARTS):
        s = flat[h * STRIPE_BYTES : (h + 1) * STRIPE_BYTES].sum()
        out[h] = np.uint32(s & 0xFFFFFFFF)
    return out


# ------------------------------------------------------------------
# Comparison + reporting
# ------------------------------------------------------------------
def compare(device_hist: np.ndarray, device_csums: np.ndarray,
            summary: dict, reference_hist: np.ndarray,
            reference_csums: np.ndarray):
    errors = []

    # Per-hart stripe checksum mismatch is the most specific
    # diagnostic we have: each hart's checksum and its contribution
    # to the histogram come from the SAME byte reads in Phase 0, so
    # a checksum delta of magnitude M means that hart's stripe
    # reads were off by M (i.e. one or more bytes returned wrong
    # values). Report per-hart deltas before the aggregate
    # histogram diff so the launcher output pins the failing hart.
    csum_diff = device_csums.astype(np.int64) - reference_csums.astype(np.int64)
    bad_harts = np.nonzero(csum_diff)[0]
    if bad_harts.size:
        samples = [
            f"hart {int(h):2d}: device=0x{int(device_csums[h]):08X} "
            f"reference=0x{int(reference_csums[h]):08X} "
            f"(delta={int(csum_diff[h]):+d})"
            for h in bad_harts
        ]
        errors.append(
            f"stripe checksums: {bad_harts.size}/{NUM_HARTS} harts disagree. "
            f"Indicates Phase-0 stripe reads saw wrong bytes:\n    "
            + "\n    ".join(samples)
        )

    if summary["magic"] != MAGIC:
        errors.append(
            f"magic: 0x{summary['magic']:08X} != expected 0x{MAGIC:08X}"
        )
    if (summary["width"], summary["height"]) != (IMG_W, IMG_H):
        errors.append(
            f"dims: {summary['width']}x{summary['height']} != {IMG_W}x{IMG_H}"
        )
    if summary["total_pixels"] != TOTAL_PIXELS:
        errors.append(
            f"total_pixels: {summary['total_pixels']} != {TOTAL_PIXELS}"
        )
    if summary["sum_of_bins"] != summary["total_pixels"]:
        errors.append(
            f"sum_of_bins {summary['sum_of_bins']} != "
            f"total_pixels {summary['total_pixels']}"
        )
    if int(device_hist.sum()) != TOTAL_PIXELS:
        errors.append(
            f"device histogram sums to {int(device_hist.sum())} != {TOTAL_PIXELS}"
        )

    diff = device_hist.astype(np.int64) - reference_hist.astype(np.int64)
    mismatched = int(np.count_nonzero(diff))
    if mismatched:
        # Sample up to 10 mismatches for the report
        bad_idx = np.nonzero(diff)[0]
        samples = [
            f"bin {int(i):3d}: device={int(device_hist[i])} "
            f"reference={int(reference_hist[i])} (diff={int(diff[i])})"
            for i in bad_idx[:10]
        ]
        errors.append(
            f"histogram: {mismatched}/{BINS} bins differ. First mismatches:\n    "
            + "\n    ".join(samples)
        )

    ref_max_idx = int(reference_hist.argmax())
    ref_max_count = int(reference_hist[ref_max_idx])
    if summary["max_count"] != ref_max_count or summary["max_index"] != ref_max_idx:
        errors.append(
            f"max: device (count={summary['max_count']}, "
            f"bin={summary['max_index']}) != reference "
            f"(count={ref_max_count}, bin={ref_max_idx})"
        )

    return errors, mismatched


def write_summary_txt(out: Path, summary: dict, errors: list, ref: np.ndarray):
    lines = []
    lines.append("=== Launcher summary ===")
    lines.append(f"  input size       : {IMG_W}x{IMG_H}")
    lines.append(f"  expected magic   : 0x{MAGIC:08X}")
    lines.append("")
    lines.append("=== Kernel summary (parsed from dump) ===")
    lines.append(f"  magic            : 0x{summary['magic']:08X}")
    lines.append(f"  width x height   : {summary['width']} x {summary['height']}")
    lines.append(f"  total_pixels     : {summary['total_pixels']}")
    lines.append(f"  sum_of_bins      : {summary['sum_of_bins']}")
    lines.append(f"  max_count        : {summary['max_count']}")
    lines.append(f"  max_index        : {summary['max_index']}")
    lines.append(f"  _reserved        : 0x{summary['_reserved']:08X}")
    lines.append("")
    lines.append("=== Reference stats ===")
    lines.append(f"  sum              : {int(ref.sum())}")
    lines.append(f"  max              : {int(ref.max())} at bin {int(ref.argmax())}")
    lines.append(f"  min              : {int(ref.min())} at bin {int(ref.argmin())}")
    lines.append(f"  mean             : {float(ref.mean()):.2f}")
    lines.append("")
    if errors:
        lines.append("=== FAIL ===")
        for e in errors:
            lines.append(f"  - {e}")
    else:
        lines.append("=== PASS ===")
    out.write_text("\n".join(lines) + "\n")


def write_histogram_txt(out: Path, dev: np.ndarray, ref: np.ndarray):
    diff = dev.astype(np.int64) - ref.astype(np.int64)
    with out.open("w") as f:
        f.write("bin  device    reference diff\n")
        f.write("---- --------- --------- ----\n")
        for i in range(BINS):
            f.write(f"{i:3d}  {int(dev[i]):9d} {int(ref[i]):9d} {int(diff[i]):+d}\n")


def plot_histograms(png: Path, dev: np.ndarray, ref: np.ndarray, img: np.ndarray):
    fig, (ax_img, ax_hist, ax_diff) = plt.subplots(
        3, 1, figsize=(12, 12), constrained_layout=True
    )

    ax_img.imshow(img, cmap="gray", vmin=0, vmax=255, interpolation="nearest")
    ax_img.set_title(f"Input image ({IMG_W}x{IMG_H} grayscale)")
    ax_img.set_xticks([])
    ax_img.set_yticks([])

    bins = np.arange(BINS)
    ax_hist.bar(bins, ref, width=1.0, alpha=0.6, label="reference (host)")
    ax_hist.bar(bins, dev, width=1.0, alpha=0.6, label="device (erbium)")
    ax_hist.set_xlabel("bin (pixel value)")
    ax_hist.set_ylabel("count")
    ax_hist.set_title("Histogram: device vs reference")
    ax_hist.legend()
    ax_hist.set_xlim(0, BINS - 1)

    diff = dev.astype(np.int64) - ref.astype(np.int64)
    ax_diff.bar(bins, diff, width=1.0, color="tab:red")
    ax_diff.axhline(0, color="black", linewidth=0.5)
    ax_diff.set_xlabel("bin")
    ax_diff.set_ylabel("device - reference")
    ax_diff.set_title("Per-bin difference (zero everywhere on PASS)")
    ax_diff.set_xlim(0, BINS - 1)

    fig.savefig(png, dpi=120)
    plt.close(fig)


# ------------------------------------------------------------------
# Main
# ------------------------------------------------------------------
def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--device",
                    choices=["soc1sim", "sys_emu", "erbium_emu"],
                    default="soc1sim",
                    help="target device (default: soc1sim)")
    ap.add_argument("--launcher", type=str, default=DEFAULT_LAUNCHER,
                    help=f"path to erbium_run wrapper (default: {DEFAULT_LAUNCHER})")
    ap.add_argument("--elf", type=Path, default=None,
                    help="path to histogram.{erbium,erbium-soc1sim}.elf "
                         "(default: auto per --device)")
    ap.add_argument("--image", type=Path, default=None,
                    help="optional input image (PNG/JPG); auto-converted to "
                         f"{IMG_W}x{IMG_H} grayscale. If omitted, either "
                         "--preset selects a bundled image, or a deterministic "
                         "synthetic image is generated.")
    ap.add_argument("--preset", choices=sorted(PRESETS.keys()), default=None,
                    help="pick a bundled test image from erbium-examples/testdata/ "
                         f"({', '.join(sorted(PRESETS.keys()))}). Ignored if "
                         "--image is given.")
    ap.add_argument("--seed", type=int, default=42,
                    help="RNG seed for the synthetic fallback image (default: 42)")
    ap.add_argument("--output-dir", type=Path, default=Path("./results"),
                    help="directory where all artifacts are written "
                         "(default: ./results)")
    args, extra = ap.parse_known_args()

    elf = args.elf or DEFAULT_ELFS.get(args.device, DEFAULT_ELFS["soc1sim"])

    if not elf.exists():
        print(f"error: histogram ELF not found at {elf}\n"
              f"       build it first (target: {elf.name})",
              file=sys.stderr)
        return 1

    outdir = args.output_dir
    outdir.mkdir(parents=True, exist_ok=True)

    image_bin = outdir / "input.bin"
    image_png = outdir / "input.png"
    dump_bin = outdir / "dump_after.bin"
    stdout_path = outdir / "launcher.stdout"
    stderr_path = outdir / "launcher.stderr"
    summary_txt = outdir / "summary.txt"
    hist_txt = outdir / "histogram.txt"
    hist_png = outdir / "histogram.png"

    if args.image:
        img = load_image(args.image)
        print(f"loaded image from {args.image} -> {IMG_W}x{IMG_H} grayscale")
    elif args.preset:
        src = PRESETS[args.preset]
        if not src.exists():
            print(f"error: preset image missing at {src} — see testdata/README.md",
                  file=sys.stderr)
            return 1
        img = load_image(src)
        print(f"loaded preset '{args.preset}' from {src} -> {IMG_W}x{IMG_H} grayscale")
    else:
        img = generate_image(args.seed)
        print(f"generated synthetic image (seed={args.seed}, {IMG_W}x{IMG_H} grayscale)")

    image_bin.write_bytes(img.tobytes())
    save_image_png(img, image_png)
    print(f"wrote {image_bin} ({image_bin.stat().st_size} bytes)")
    print(f"wrote {image_png}")

    ref_hist = np.bincount(img.flatten(), minlength=BINS).astype(np.uint64)

    rc = run_launcher(args.launcher, elf, image_bin, dump_bin, stdout_path, stderr_path,
                      device=args.device, extra=extra)
    if rc != 0:
        print(f"launcher exited with {rc}; see {stderr_path}", file=sys.stderr)
        return 1

    if not dump_bin.exists():
        print(f"error: {dump_bin} was not produced", file=sys.stderr)
        return 1

    dev_hist, dev_csums, summary = parse_dump(dump_bin)
    ref_csums = reference_stripe_csums(img)
    errors, mismatched = compare(dev_hist, dev_csums, summary,
                                 ref_hist, ref_csums)

    write_summary_txt(summary_txt, summary, errors, ref_hist)
    write_histogram_txt(hist_txt, dev_hist, ref_hist)
    plot_histograms(hist_png, dev_hist, ref_hist, img)

    print()
    print(f"Artifacts under {outdir.resolve()}:")
    for p in (image_png, image_bin, dump_bin, summary_txt, hist_txt, hist_png,
              stdout_path, stderr_path):
        print(f"  {p.name}")
    print()
    if errors:
        print("RESULT: FAIL")
        for e in errors:
            print(f"  - {e.splitlines()[0]}")  # just the first line in stdout
        print(f"(see {summary_txt} for full report)")
        return 1

    print("RESULT: PASS")
    print(f"  {mismatched} bins differ (all zero)")
    print(f"  max bin = {int(ref_hist.argmax())} with {int(ref_hist.max())} pixels")
    return 0


if __name__ == "__main__":
    sys.exit(main())
