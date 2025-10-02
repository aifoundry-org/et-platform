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
### Security

## [1.7.0] - 2024-05-17
### Added
### Changed
- [CI] updated docker to ubuntu 22.04
### Deprecated
### Removed
### Fixed
### Security

## [1.6.0] - 2023-11-07
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-18522] Minor build fix for clang
### Security

## [1.5.0] - 2023-08-03
### Added
### Changed
- [SW-17875] Update conanfile.py and CI to gitlab.com
- Updated conan recipe to be Conan 2.0 ready
### Deprecated
### Removed
### Fixed
### Security

## [1.4.0] - 2023-03-16
### Added
- Adding PCIe Phy register definition
### Changed
- [SW-16450] Generate pre-releases by default and full SemVer pkgs on tags pipelines.
- [SW-15063] Moving out non-generic C flags (-fno-zero-initialized-in-bss -ffunction-sections -fdata-sections) from toolchain config to consumer projects
### Deprecated
### Removed
### Fixed
### Security

## [1.3.0] - 2022-10-21
### Added
### Changed
- Updated memshire configuration to enable PCODE updates for DBI, timing, and disabling self-refresh.
### Deprecated
### Removed
### Fixed
### Security

## [1.2.0] - 2022-09-02
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
- [SW-13778] New HPDPLL modes added
### Changed
### Deprecated
### Removed
### Fixed
- Included rules/generic-workflow-conan.yaml to enable execution of verify_project_changes pre job
### Security

## [1.1.0] - 2022-7-4
### Added
- HDPLL modes added.
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
