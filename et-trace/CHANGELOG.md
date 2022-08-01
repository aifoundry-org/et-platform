# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
- Added resource utilization stats support
### Changed
- Made Trace string length as configurable.
- Adapt conan pipeline to conan 1.49
### Deprecated
### Removed
### Fixed
- [SW-13631] Aligned trace standard header to cache-line size.
### Security

## [0.5.0] - 2022-6-22
### Added
### Changed
- Trace buffer trace lock acquire/release functions to runtime.
### Deprecated
### Removed
### Fixed
### Security

## [0.4.0] - 2022-06-08
### Added
- Custom events IDs new structs for Health Monitor events.
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.3.0] - 2022-05-30
### Added
- Custom events IDs for SP, MM and CM.
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.2.0] - 2022-05-25
### Added
- Support for variable length string message.
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.1.0] - 2022-03-08
Initial version
### Added
- This CHANGELOG file.
- Conan recipe to package the project.
### Changed
### Deprecated
### Removed
### Fixed
### Security

[Unreleased]: https://gitlab.esperanto.ai/software/et-trace/-/compare/v0.3.0...master
[0.3.0]: https://gitlab.esperanto.ai/software/et-trace/-/compare/v0.3.0...master
[0.2.0]: https://gitlab.esperanto.ai/software/et-trace/-/compare/v0.1.0...v0.2.0
[0.1.0]: https://gitlab.esperanto.ai/software/et-trace/-/tags/v0.1.0
