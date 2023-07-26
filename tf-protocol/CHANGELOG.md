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
[CI] Adapt pipeline and conanfile.py to gitlab.com
### Security

## [1.2.0] - 2022-11-22
### Added
### Changed
- [SW-14614] Changed wait duration cycles from 32-bits to 64-bits
- [SW-9608] Updated TRACE RT control/configure IDs
### Deprecated
### Removed
### Fixed
### Security

## [1.1.0] - 2022-10-20
### Added
### Changed
- [SW-14154] Updating Kernel TF specification according to ops api
### Deprecated
### Removed
### Fixed
### Security

## [1.0.0] - 2022-09-07
### Added
### Changed
- [SW-14044] Update module power to be 2 bytes in module power structure
    - This is a breaking change, TF_RSP_PMIC_MODULE_POWER now returns 16-bit power value instead of 8-bit.
### Deprecated
### Removed
### Fixed
### Security

## [0.2.0] - 2022-09-05
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.1.0] - 2022-6-22
### Added
- Initial version
- This CHANGELOG file.
- A README file.
- Conan recipe to package the project.
### Changed
### Deprecated
### Removed
### Fixed
### Security


[Unreleased]: https://gitlab.esperanto.ai/software/tf-protocol/-/compare/v0.1.0...master
[0.1.0]: https://gitlab.esperanto.ai/software/tf-protocol/-/tags/v0.1.0
