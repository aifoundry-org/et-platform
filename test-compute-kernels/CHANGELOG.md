# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
### Changed
- [CI] updated docker to ubuntu 22.04
### Deprecated
### Removed
### Fixed
### Security

## [1.12.0] - 2024-03-27
### Added
### Changed
- Improved B/W test
### Deprecated
### Removed
### Fixed
### Security

## [1.11.0] - 2024-01-30
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-19533] Add support to the kernels to be clang compatible
- [SW-19783] Added clang loop unroll support
### Security

## [1.10.0] - 2023-12-07
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-19534] Align the global array in the BSS test to cache line size
### Security

## [1.9.0] - 2023-11-07
### Added
- [SW-18547] Adding custom stack kernel.
### Changed
### Deprecated
### Removed
### Fixed
- [SW-18876] Fix the clang reported errors to make the kernels clang ready
- [SW-18876] Fix compiler generated warnings
- [SW-18944] Reduce clang compile time on tl_tfma_fc
### Security

## [1.8.0] - 2023-09-01
### Added
- [SW-18383] Improve logging in TIMA virus kernel.
- Improve logging in TIMA virus kernel.
### Changed
- Added printf for first hart on Virus test
### Deprecated
### Removed
### Fixed
- Add tensor FMA wait for TIMA to complete.
### Security

## [1.7.0] - 2023-08-03
### Added
### Changed
- Updating the conan dependencies versions to released components
### Deprecated
### Removed
### Fixed
[CI] Adapt pipeline and conanfile.py to gitlab.com
### Security

## [1.6.0] - 2023-03-15
### Added
- [SW-16253] Added environment kernel for verifying the kernel env params.
- [SW-15063] Moving out non-generic C flags (-fno-zero-initialized-in-bss -ffunction-sections -fdata-sections) from toolchain config to consumer projects
### Changed
- [SW-16520] Generate pre-release versions by default and full versions in tags pipelines
### Deprecated
### Removed
### Fixed
### Security

## [1.5.0] - 2023-01-10
### Added
### Changed
- Updating esperantoTrace version in Conan.
- Revert "Disabling SS bundle phase to break dependency chain."
### Deprecated
### Removed
### Fixed
### Security

## [1.4.0] - 2022-12-15
### Added
### Changed
- Update conanfile requirements to use version-ranges
### Deprecated
### Removed
### Fixed
[SW-15362]: Fixed bug in PMC kernel
### Security

## [1.3.0] - 2022-09-02
### Added
### Changed
- [SW-13248] Modifying bus error kernel to access non-existent ESR in U-mode.
- [SW-14007] Modifying bus error kernel to generate bus error interrupt on every hart.
- [SW-14007] Increasing the artifacts expiry from 1h to 1d in Gitlab CI.
- Bumping et-common-libs and esperantoTrace version in Conan.
- [SW-10308] separate out SC and MS APIs to log PMC events
### Deprecated
### Removed
### Fixed
### Security

## [1.2.0] - 2022-8-16
### Added
- [SW-13005] Workaround for MLP kernel.
- Added new Virus Kernels for Power measurement
### Changed
### Deprecated
### Removed
### Fixed
- [SW-13233] Coherency hazards fixed in MLP kernel and removed workaround.
- [SW-13685] FLB checker issues fixed in MLP kernel.
- Fix bug to remove cobbler which was causing registers to be backed up
### Security

## [1.1.0] - 2022-7-4
### Added
- Fixed bus error kernel, setting stride to 0
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [1.0.0] - 2022-6-23
### Added
- Initial version of changelog file.
### Changed
### Deprecated
### Removed
### Fixed
### Security
