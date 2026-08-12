"""Run a curated GP-SDK silicon suite through the LPDDR single-neighborhood Erbium path."""

from pathlib import Path
import os
import shlex
import pytest

from lpddr_erbium_cases import ALL_CASES


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SHIRE_MASK = "0x200"
DEFAULT_ACTIVE_NEIGHBORHOOD = "0"


def _env_prefix() -> str:
    root = str(ROOT)
    return " ".join(
        [
            "env",
            f"LD_LIBRARY_PATH={root}/build/esperanto-tools-libs-prefix/src/esperanto-tools-libs-build:/opt/et/lib",
            "ET_SKIP_INIT_ABORT=1",
            "ET_SKIP_DEVICE_API_CHECK=1",
            "ET_DISABLE_KERNEL_TRACES=1",
            "ET_EXECUTION_CONTEXT_CACHE_PREALLOC=0",
            "GPSDK_SKIP_LOAD_WAIT=1",
            "GPSDK_LOAD_QUIESCE_MS=1000",
        ]
    )


def _common_lpddr_args() -> list[str]:
    return [
        "--device_type=silicon",
        f"--shire_mask={os.environ.get('GPSDK_LPDDR_ERBIUM_SHIRE_MASK', DEFAULT_SHIRE_MASK)}",
        f"--active_neighborhood={os.environ.get('GPSDK_LPDDR_ERBIUM_ACTIVE_NEIGHBORHOOD', DEFAULT_ACTIVE_NEIGHBORHOOD)}",
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
    args.extend(_common_lpddr_args())
    return f"{_env_prefix()} " + " ".join(shlex.quote(arg) for arg in args)


@pytest.mark.parametrize("case", ALL_CASES, ids=lambda case: case.name)
def test_run_lpddr_erbium_examples(shell, build_dir, case):
    """Run GP-SDK kernels on silicon with the single-neighborhood LPDDR Erbium path."""
    if not build_dir.exists():
        pytest.skip("the examples have not been built")
    if case.skip_reason:
        pytest.skip(case.skip_reason)

    shell.run(_build_command(build_dir, case))
