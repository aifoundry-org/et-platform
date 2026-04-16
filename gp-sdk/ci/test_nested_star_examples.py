"""Run a curated GP-SDK silicon suite through the nested-star Erbium-sim launcher path."""

from pathlib import Path
import os
import shlex
import pytest

from nested_star_cases import ALL_CASES


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SHIRE_MASK = "0x200"
DEFAULT_ACTIVE_NEIGHBORHOOD = "0"


def _env_prefix() -> str:
    root = str(ROOT)
    return " ".join(
        [
            "env",
            f"LD_LIBRARY_PATH={root}/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib",
            "ET_DISABLE_KERNEL_TRACES=1",
            "ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0",
            "ET_SKIP_MEMCPY_DEVICE_CHECK=1",
        ]
    )


def _common_nested_args(build_dir) -> list[str]:
    topology_probe = build_dir.device / "tests" / "shire_latency_probe.elf_dbg"
    return [
        "--device_type=silicon",
        f"--shire_mask={os.environ.get('GPSDK_NESTED_STAR_SHIRE_MASK', DEFAULT_SHIRE_MASK)}",
        f"--active_neighborhood={os.environ.get('GPSDK_NESTED_STAR_ACTIVE_NEIGHBORHOOD', DEFAULT_ACTIVE_NEIGHBORHOOD)}",
        "--scratchpad_nested_star",
        "--erbium_sim",
        f"--topology_probe_kernel={topology_probe}",
    ]


def _build_command(build_dir, case) -> str:
    launcher = build_dir.host / "sdk" / case.launcher
    kernel = build_dir.device / "tests" / case.kernel
    args = [
        str(launcher),
        f"--kernel_path={kernel}",
        f"--kernel_launch_timeout={case.timeout}",
    ]
    args.extend(case.extra_args)
    args.extend(_common_nested_args(build_dir))
    return f"{_env_prefix()} " + " ".join(shlex.quote(arg) for arg in args)


@pytest.mark.parametrize("case", ALL_CASES, ids=lambda case: case.name)
def test_run_nested_star_examples(shell, build_dir, case):
    """Run GP-SDK kernels and nested-star demos on silicon with the nested-star launcher path."""
    if not build_dir.exists():
        pytest.skip("the examples have not been built")
    if case.skip_reason:
        pytest.skip(case.skip_reason)

    shell.run(_build_command(build_dir, case))
