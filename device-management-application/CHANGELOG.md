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

## [1.2.0] - 2022-7-7
### Added
- ET-Top: Add -f dump trace stats buffer option and version info.
### Changed
- ET-Top: Remove -f dump trace stats buffer option and add interactive dump command.
- ET-Top: Add displayOpStats helper and add conversion from milliwatts to watts.
### Deprecated
### Removed
- ET-Top: Remove use of routines from dlfcn.h.
### Fixed
- ET-Top: Decode until you find a valid packet.
### Security

## [1.1.0] - 2022-7-4
### Added
- Gitlab CI with clang format job along with sw-platform regressions
- Added -v (version) argument to get DM application version
- Clang formatter rules.
- Added support to extract SP and MM stats trace buffer
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
- Changed back the sw-platform branch for CI back to develop/system-sw
- Ran clang formatter.
### Deprecated
### Removed
### Fixed
- Clang format to include .cc file type.
### Security

## [1.0.0] - 2022-6-24
### Added
- This CHANGELOG file.
### Changed
### Deprecated
### Removed
- Redundant readme file.
### Fixed
### Security
