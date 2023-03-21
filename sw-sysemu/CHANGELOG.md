# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Fixed et_trace to 1.2.1
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
