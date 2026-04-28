#!/usr/bin/env python3
"""Run the LPDDR single-neighborhood GP-SDK silicon suite directly, without pytest."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys


REPO_ROOT = Path(__file__).resolve().parents[3]
CI_DIR = REPO_ROOT / "gp-sdk" / "ci"
sys.path.insert(0, str(CI_DIR))

from lpddr_erbium_cases import ALL_CASES, POSITIVE_CASES  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the LPDDR single-neighborhood GP-SDK silicon suite.")
    parser.add_argument("--repo-root", type=Path, default=REPO_ROOT)
    parser.add_argument("--host-build", type=Path,
                        default=REPO_ROOT / "build/host-prefix/src/host-build")
    parser.add_argument("--device-build", type=Path,
                        default=REPO_ROOT / "build/device-prefix/src/device-build")
    parser.add_argument("--shire-mask", default="0x200")
    parser.add_argument("--active-neighborhood", default="0")
    parser.add_argument("--case", action="append", default=[],
                        help="Run only the named case. May be passed multiple times.")
    parser.add_argument("--list", action="store_true", help="List available cases and exit.")
    parser.add_argument("--include-skipped", action="store_true",
                        help="Attempt cases that are skipped by default.")
    return parser.parse_args()


def build_env(repo_root: Path) -> dict[str, str]:
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = f"{repo_root}/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib"
    env["ET_SKIP_INIT_ABORT"] = "1"
    env["ET_SKIP_DEVICE_API_CHECK"] = "1"
    env["ET_DISABLE_KERNEL_TRACES"] = "1"
    env["ET_EXECUTION_CONTEXT_CACHE_PREALLOC"] = "0"
    env["GPSDK_SKIP_LOAD_WAIT"] = "1"
    env["GPSDK_LOAD_QUIESCE_MS"] = "1000"
    return env


def build_command(args: argparse.Namespace, case) -> list[str]:
    launcher = args.host_build / "sdk" / case.launcher
    kernel = args.device_build / "tests" / case.kernel
    return [
        str(launcher),
        f"--kernel_path={kernel}",
        f"--kernel_launch_timeout={case.timeout}",
        *case.extra_args,
        "--device_type=silicon",
        f"--shire_mask={args.shire_mask}",
        f"--active_neighborhood={args.active_neighborhood}",
    ]


def iter_cases(args: argparse.Namespace):
    cases = ALL_CASES if args.include_skipped else POSITIVE_CASES
    if args.case:
        allowed = set(args.case)
        cases = [case for case in cases if case.name in allowed]
    return cases


def main() -> int:
    args = parse_args()
    cases = iter_cases(args)
    if args.list:
        for case in ALL_CASES:
            status = "skip" if case.skip_reason else "run"
            print(f"{case.name:32} {status}  {case.skip_reason or ''}".rstrip())
        return 0

    if not cases:
        print("No matching cases selected.", file=sys.stderr)
        return 2

    env = build_env(args.repo_root)
    failures = []
    skipped = []

    for case in cases:
        if case.skip_reason and not args.include_skipped:
            skipped.append(case.name)
            print(f"SKIP {case.name}: {case.skip_reason}")
            continue

        cmd = build_command(args, case)
        print(f"RUN  {case.name}: {' '.join(shlex.quote(arg) for arg in cmd)}")
        proc = subprocess.run(cmd, cwd=args.repo_root, env=env, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True)
        if proc.returncode == 0:
            print(f"PASS {case.name}")
        else:
            failures.append(case.name)
            print(f"FAIL {case.name} rc={proc.returncode}")
            lines = proc.stdout.splitlines()
            tail = "\n".join(lines[-20:])
            if tail:
                print(tail)

    print(f"\nSummary: pass={len(cases) - len(failures)} fail={len(failures)} skip={len(skipped)}")
    if failures:
        print("Failing cases:", ", ".join(failures))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
