"""Shared LPDDR single-neighborhood Erbium-sim silicon suite manifest."""

from dataclasses import dataclass, field


@dataclass(frozen=True)
class LpddrErbiumCase:
    name: str
    launcher: str
    kernel: str
    timeout: int = 30
    extra_args: tuple[str, ...] = field(default_factory=tuple)
    skip_reason: str | None = None


POSITIVE_CASES = [
    LpddrErbiumCase("print", "basic_launcher", "print.elf_dbg"),
    LpddrErbiumCase("print2", "basic_launcher", "print2.elf_dbg"),
    LpddrErbiumCase("bss", "basic_launcher", "bss.elf_dbg", extra_args=("--num_launches=5",)),
    LpddrErbiumCase("c_tls", "basic_launcher", "c_tls.elf_dbg"),
    LpddrErbiumCase("cpp_tls", "basic_launcher", "cpp_tls.elf_dbg"),
    LpddrErbiumCase("external_tls", "basic_launcher", "external_tls.elf_dbg"),
    LpddrErbiumCase("gp", "basic_launcher", "gp.elf_dbg"),
    LpddrErbiumCase("txfma", "txfma_launcher", "txfma.elf_dbg"),
]


SKIPPED_CASES = [
    LpddrErbiumCase("data", "basic_launcher", "data.elf_dbg", extra_args=("--num_launches=5",),
                    skip_reason="Fails with Kernel Launch Error (error code: 4) under LPDDR single-neighborhood execution."),
    LpddrErbiumCase("c_constructors", "basic_launcher", "c_constructors.elf_dbg",
                    skip_reason="KernelLaunchUnexpectedError under LPDDR single-neighborhood silicon runs."),
    LpddrErbiumCase("cpp_constructors", "basic_launcher", "cpp_constructors.elf_dbg",
                    skip_reason="KernelLaunchUnexpectedError under LPDDR single-neighborhood silicon runs."),
    LpddrErbiumCase("saxpy_scalar", "saxpy_launcher", "saxpy_scalar.elf_dbg",
                    skip_reason="Host/device results do not match under LPDDR single-neighborhood execution."),
    LpddrErbiumCase("saxpy_vector", "saxpy_launcher", "saxpy_vector.elf_dbg",
                    skip_reason="Host/device results do not match under LPDDR single-neighborhood execution."),
    LpddrErbiumCase("saxpy_intrinsics", "saxpy_launcher", "saxpy_intrinsics.elf_dbg",
                    skip_reason="Host/device results do not match under LPDDR single-neighborhood execution."),
    LpddrErbiumCase("sdot_scalar", "sdot_launcher", "sdot_scalar.elf_dbg",
                    skip_reason="Host/device results do not match under LPDDR single-neighborhood execution."),
    LpddrErbiumCase("sdot_vector", "sdot_launcher", "sdot_vector.elf_dbg",
                    skip_reason="Host/device results do not match under LPDDR single-neighborhood execution."),
    LpddrErbiumCase("autogen_matmul", "matmul_launcher", "autogen_matmul.elf_dbg", timeout=120,
                    skip_reason="Hangs after runtime initialization under LPDDR single-neighborhood execution."),
    LpddrErbiumCase("variableStrings", "basic_launcher", "variableStrings.elf_dbg", timeout=60,
                    skip_reason="Currently hangs under LPDDR single-neighborhood silicon runs."),
    LpddrErbiumCase("fft", "fft_launcher", "fftKernel.elf_dbg",
                    skip_reason="Current host launcher does not accept single-shire launch arguments."),
    LpddrErbiumCase("syncAll", "basic_launcher", "syncAll.elf_dbg",
                    skip_reason="Barrier semantics are outside the single-neighborhood Erbium microcontroller model."),
    LpddrErbiumCase("syncDeviceBasic", "barrier_launcher", "syncDeviceBasic.elf_dbg",
                    skip_reason="Barrier launcher path is outside the single-neighborhood Erbium microcontroller model."),
    LpddrErbiumCase("syncMinion", "barrier_launcher", "syncMinion.elf_dbg",
                    skip_reason="Barrier launcher path is outside the single-neighborhood Erbium microcontroller model."),
    LpddrErbiumCase("syncShire2EP", "barrier_launcher", "syncShire2EP.elf_dbg",
                    skip_reason="Barrier launcher path is outside the single-neighborhood Erbium microcontroller model."),
    LpddrErbiumCase("user_defined_stack", "stack_launcher", "user_defined_stack.elf_dbg",
                    extra_args=("--stackSize=8192",),
                    skip_reason="Custom per-hart stack bring-up is not yet validated in the LPDDR Erbium mode."),
    LpddrErbiumCase("busy10sec", "basic_launcher", "busy10sec.elf_dbg",
                    skip_reason="Timing-only busywait kernel; not part of the functional Erbium suite."),
    LpddrErbiumCase("cacheops_flush", "basic_launcher", "cacheops_flush.elf_dbg",
                    skip_reason="Cache maintenance validation is not yet part of the LPDDR Erbium functional suite."),
    LpddrErbiumCase("profiling_simple", "basic_launcher", "profiling_simple.elf_dbg",
                    skip_reason="Profiling-oriented kernel; not part of the LPDDR Erbium functional suite."),
    LpddrErbiumCase("profiling_stress", "basic_launcher", "profiling_stress.elf_dbg",
                    skip_reason="Profiling-oriented kernel; not part of the LPDDR Erbium functional suite."),
    LpddrErbiumCase("tracing_busywait", "basic_launcher", "tracing_busywait.elf_dbg",
                    skip_reason="Tracing validation is out of scope when kernel traces are disabled."),
    LpddrErbiumCase("tracing_factorial", "basic_launcher", "tracing_factorial.elf_dbg",
                    skip_reason="Tracing validation is out of scope when kernel traces are disabled."),
    LpddrErbiumCase("nested_scratchpad_stencil", "nested_scratchpad_stencil_demo",
                    "nested_scratchpad_stencil.elf_dbg",
                    skip_reason="Nested-scratchpad demo belongs to the abandoned SCP-backed approach."),
    LpddrErbiumCase("nested_scratchpad_atomic_reads", "nested_scratchpad_atomic_reads_demo",
                    "nested_scratchpad_atomic_reads.elf_dbg",
                    skip_reason="Nested-scratchpad demo belongs to the abandoned SCP-backed approach."),
    LpddrErbiumCase("nested_scratchpad_chimera_gemv", "nested_scratchpad_chimera_gemv_demo",
                    "nested_scratchpad_chimera_gemv.elf_dbg",
                    skip_reason="Nested-scratchpad demo belongs to the abandoned SCP-backed approach."),
    LpddrErbiumCase("erbium_sim_access_violation", "erbium_sim_access_violation_demo",
                    "erbium_sim_access_violation.elf_dbg",
                    skip_reason="Scratchpad-fence negative test belongs to the abandoned SCP-backed approach."),
    LpddrErbiumCase("topology_probe", "shire_latency_probe", "shire_latency_probe.elf_dbg",
                    skip_reason="Topology probing belongs to the abandoned nested-star scratchpad path."),
    LpddrErbiumCase("scp_exec_probe", "scp_exec_probe_demo", "scp_exec_probe.elf_dbg",
                    skip_reason="SCP execution probe documents the rejected SCP code path, not the LPDDR path."),
]


ALL_CASES = POSITIVE_CASES + SKIPPED_CASES
