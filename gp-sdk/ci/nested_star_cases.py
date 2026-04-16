"""Shared nested-star silicon suite manifest."""

from dataclasses import dataclass, field


@dataclass(frozen=True)
class NestedStarCase:
    name: str
    launcher: str
    kernel: str
    timeout: int = 30
    extra_args: tuple[str, ...] = field(default_factory=tuple)
    skip_reason: str | None = None


POSITIVE_CASES = [
    NestedStarCase("print", "basic_launcher", "print.elf_dbg"),
    NestedStarCase("print2", "basic_launcher", "print2.elf_dbg"),
    NestedStarCase("bss", "basic_launcher", "bss.elf_dbg", extra_args=("--num_launches=5",)),
    NestedStarCase("data", "basic_launcher", "data.elf_dbg", extra_args=("--num_launches=5",)),
    NestedStarCase("c_tls", "basic_launcher", "c_tls.elf_dbg"),
    NestedStarCase("cpp_tls", "basic_launcher", "cpp_tls.elf_dbg"),
    NestedStarCase("external_tls", "basic_launcher", "external_tls.elf_dbg"),
    NestedStarCase("gp", "basic_launcher", "gp.elf_dbg"),
    NestedStarCase("nested_scratchpad_stencil", "nested_scratchpad_stencil_demo", "nested_scratchpad_stencil.elf_dbg",
                   timeout=120),
    NestedStarCase("nested_scratchpad_atomic_reads", "nested_scratchpad_atomic_reads_demo",
                   "nested_scratchpad_atomic_reads.elf_dbg", timeout=120),
    NestedStarCase("nested_scratchpad_chimera_gemv", "nested_scratchpad_chimera_gemv_demo",
                   "nested_scratchpad_chimera_gemv.elf_dbg", timeout=180),
    NestedStarCase("erbium_sim_access_violation", "erbium_sim_access_violation_demo",
                   "erbium_sim_access_violation.elf_dbg", timeout=120),
]


SKIPPED_CASES = [
    NestedStarCase("syncAll", "basic_launcher", "syncAll.elf_dbg",
                   skip_reason="Kernel barrier assumptions do not hold under single-neighborhood nested-star execution."),
    NestedStarCase("syncDeviceBasic", "barrier_launcher", "syncDeviceBasic.elf_dbg",
                   skip_reason="Barrier launcher aborts during kernel launch under nested-star silicon runs."),
    NestedStarCase("syncMinion", "barrier_launcher", "syncMinion.elf_dbg",
                   skip_reason="Barrier launcher aborts during kernel launch under nested-star silicon runs."),
    NestedStarCase("user_defined_stack", "stack_launcher", "user_defined_stack.elf_dbg",
                   extra_args=("--stackSize=8192",),
                   skip_reason="Custom per-hart stack setup is not yet compatible with nested-star execution."),
    NestedStarCase("saxpy_scalar", "saxpy_launcher", "saxpy_scalar.elf_dbg",
                   skip_reason="Kernel faults under nested-star single-neighborhood execution."),
    NestedStarCase("saxpy_vector", "saxpy_launcher", "saxpy_vector.elf_dbg",
                   skip_reason="Kernel faults under nested-star single-neighborhood execution."),
    NestedStarCase("saxpy_intrinsics", "saxpy_launcher", "saxpy_intrinsics.elf_dbg",
                   skip_reason="Kernel faults under nested-star single-neighborhood execution."),
    NestedStarCase("sdot_scalar", "sdot_launcher", "sdot_scalar.elf_dbg",
                   skip_reason="Host runtime crashes in the D2H event path under nested-star execution."),
    NestedStarCase("sdot_vector", "sdot_launcher", "sdot_vector.elf_dbg",
                   skip_reason="Host runtime crashes in the D2H event path under nested-star execution."),
    NestedStarCase("txfma", "txfma_launcher", "txfma.elf_dbg",
                   skip_reason="Kernel faults under nested-star single-neighborhood execution."),
    NestedStarCase("c_constructors", "basic_launcher", "c_constructors.elf_dbg",
                   skip_reason="Currently faults under nested-star single-neighborhood silicon runs."),
    NestedStarCase("cpp_constructors", "basic_launcher", "cpp_constructors.elf_dbg",
                   skip_reason="Currently faults under nested-star single-neighborhood silicon runs."),
    NestedStarCase("cacheops_flush", "basic_launcher", "cacheops_flush.elf_dbg",
                   skip_reason="Currently hangs under nested-star silicon runs."),
    NestedStarCase("autogen_matmul", "matmul_launcher", "autogen_matmul.elf_dbg",
                   skip_reason="Not yet validated under single-shire nested-star execution."),
    NestedStarCase("profiling_simple", "basic_launcher", "profiling_simple.elf_dbg",
                   skip_reason="Profiling/tracing-oriented kernel; not part of the nested-star functional suite."),
    NestedStarCase("profiling_stress", "basic_launcher", "profiling_stress.elf_dbg",
                   skip_reason="Profiling/tracing-oriented kernel; not part of the nested-star functional suite."),
    NestedStarCase("tracing_busywait", "basic_launcher", "tracing_busywait.elf_dbg",
                   skip_reason="Tracing validation is out of scope when kernel traces are disabled."),
    NestedStarCase("tracing_factorial", "basic_launcher", "tracing_factorial.elf_dbg",
                   skip_reason="Tracing validation is out of scope when kernel traces are disabled."),
]


ALL_CASES = POSITIVE_CASES + SKIPPED_CASES
