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
