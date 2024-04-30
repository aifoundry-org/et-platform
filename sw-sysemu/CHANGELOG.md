# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- Adding Tensor Instructions to standalone test
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.17.0] - 2024-04-29
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.16.0] - 2024-03-27
### Added
- (CI) Add clang-format and clant-tidy jobs
- Added dummy micro-benchmark
- Added unit test for Perf
- Added a test sequence generator
- Extend instruction sequence to add rv64f, rv64i, rv64m, tensors
- Adding et-common-lib umode library inst_sequence generator
- Shell scripts to compile multiple tests/benchmark and run benchmarks
### Changed
- Detect if compiler supports -flto and enable it
- Change default symbol visibility to "hidden". Export only public interface
- Change default back to "default" while investigating deadlocks in shared libs.
- Parametrize benchmarks
- Improving performance on Tensor Store Checker
- Optimizing Tensor Store Checker is_empty performance
- Further TensorStore is_empty performance improvement
- Split benchmark into different instruction categories 
### Deprecated
### Removed
- rv64a, rv64d instructions generation (buggy)
### Fixed
### Security

## [0.15.0] - 2024-01-30
### Added
- [SW-19833] Log PC on coherency hazard
### Changed
- Updated pointers to match release SDK 1.4.0
- (CI) by default use conan-linux-ubuntu18.04-gcc7 image
- (Conan) Solve Ubuntu 22.04 linker issues using LD_LIBRARY_PATH provided by VirtualBuildEnv/VirtualRunEnv
### Deprecated
### Removed
### Fixed
### Security

## [0.14.0] - 2023-12-15
### Added
### Changed
- (Conan) Re-enable linux-ubuntu22.04-x86_64-gcc11-release builds
### Deprecated
### Removed
### Fixed
### Security

## [0.13.0] - 2023-09-01
### Added
### Changed
- Updated system-sw embedded components
- Disabed the inux-ubuntu22.04-x86_64-gcc11-release build until the issue is resolved.
### Deprecated
### Removed
### Fixed
- Flush and close ofstream on write trace_OnFatal file.
### Security

## [0.12.0] - 2023-08-03
### Added
- [SW-17597b] Some refactoring for [SW-17597].
- Started development for 0.12.0
- (Conan) Generate packages for linux-ubuntu22.04-x86_64-gcc11-release
- [SW-17595] Auto-attach gdbserver on user mode code.
- [SW-17597] Dump Trace-buffer on sysemu Fatals.
### Changed
- (CI) Update gitlab-ci-common pointer for Gitlab v15 compatibility
- Updated system-sw embedded components
- [CI] Adapt pipeline and conanfile.py to gitlab.com
- Update system-sw embedded components
### Deprecated
### Removed
### Fixed
  added back verify merge request
### Security


## [0.11.0] - 2023-05-30
### Added
- [SW-17025] Add qGetTLSAddr GDB support
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.10.0] - 2023-04-19
### Added
- ARCHSIM-696: Support for flush_va in mem_checker
### Changed
### Deprecated
### Removed
### Fixed
- Fixed et_trace to 1.2.1
- Code smells in devices
- Fixed bug in Cmakefile to point to 0.10.0
### Security

## [0.9.0] - 2023-03-07
### Added
- [ARCHSIM-697] Added a SW hint to waive write coherency errors
### Changed
- [SW-16269] Migrate sw-sysemu CI pipeline to use pre-release strategy
### Deprecated
### Removed
### Fixed
- Updated the Artifact list for embedded FW generation to match SysSW - 0.9.0
### Security

## [0.8.0] - 2023-01-11
### Added
### Changed
- (Conan) Updated embedded elfs for matching 0.8.0 release
### Deprecated
### Removed
### Fixed
### Security

## [0.7.0] - 2022-12-30
### Added
- [ARCHSIM-694] Add SDK_RELEASE build configuration
- [ARCHSIM-693] Add option to preload ELFs
- (Conan) Added preload_elfs/preload_compression parameters
- (Conan) Added parameters to override default preload_elfs versions
### Changed
- [ARCHSIM-695] Reorganize build artifacts
### Deprecated
### Removed
### Fixed
- [SW-15179] Fix tensor_wait logging
### Security

## [0.6.0] - 2022-11-22
### Added
- [ARCHSIM-692] Added a SW hint to waive read coherency errors
- BEMU: Add option to upgrade warnings to errors
- BEMU: Add DRAM size parameter
### Changed
- Add support for 'conan editable' with sw-sysemu recipe
### Deprecated
### Removed
- Reverted stale commit
### Fixed
- BEMU: Fix log format for coop TL
- Allocate sysemu instance on the heap to fix segfaults
- [ARCHSIM-685] Fixed GDB stub next/step commands
### Security

## [0.5.0] - 2022-10-20
### Added
- [SW-13986] Added pause / resume API to hint sysemu when it can put the main simulation thread to sleep, because there is nothing useful to be simulated (so avoid wasting CPU cycles doing nothing useful).
### Changed
- [SW-14481] (Conan) libsysemu component to require libfpu
- [SW-14434] (Conan) CI now generates Debug & Release pkgs
### Deprecated
### Removed
### Fixed
- BEMU: Fixed behavior of cache control extension (fill/evict)
- Runtime options (ex: -gdb) may now be set as part of --simulator-params.
### Security

## [0.4.0] - 2022-8-16
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines.
- BEMU: Checker for VPU RF accesses that may fail under certain operating conditions.
- Added checks for runtime options that may be overwritten.
### Changed
### Deprecated
### Removed
### Fixed
- BEMU: Fixed behavior of cache control extension (fill/evict)
- Runtime options (ex: -gdb) may now be set as part of --simulator-params.
### Security

## [0.3.0] - 2022-7-4
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Fixed the issue with rvtimer's write_mtimecmp() function. mtimecmp register value was
  not set correctly.
### Security

## [0.2.0] - 2022-6-23
### Added
- Initial version of changelog file.
### Changed
### Deprecated
### Removed
### Fixed
### Security

[Unreleased]: https://gitlab.esperanto.ai/software/sw-sysemu/-/compare/0.8.0...develop/sw-sysemu
[0.8.0]: https://gitlab.esperanto.ai/software/sw-sysemu/-/compare/0.7.0...0.8.0
[0.7.0]: https://gitlab.esperanto.ai/software/sw-sysemu/-/compare/0.6.0...0.7.0
[0.6.0]: https://gitlab.esperanto.ai/software/sw-sysemu/-/compare/0.5.0...0.6.0
[0.5.0]: https://gitlab.esperanto.ai/software/sw-sysemu/-/compare/0.4.0...0.5.0
[0.4.0]: https://gitlab.esperanto.ai/software/sw-sysemu/-/compare/0.3.0...0.4.0
[0.3.0]: https://gitlab.esperanto.ai/software/sw-sysemu/-/compare/0.2.0...0.3.0
[0.2.0]: https://gitlab.esperanto.ai/software/sw-sysemu/-/compare/0.1.0...0.2.0
[0.1.0]: https://gitlab.esperanto.ai/software/sw-sysemu/-/tags/0.1.0
