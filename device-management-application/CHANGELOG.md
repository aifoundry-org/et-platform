# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- Conanfile & Conan CI
- [SW-13480]: Adding memshire frequency log print
- et-power-top: Printing compute utilization stats.
### Changed
### Deprecated
### Removed
### Fixed
- Typo in conanfile url metadata
- Warnings in device management service.
### Security

## [1.4.0] - 2022-7-12
### Added
- ET-Top: Add 'w' interactive command to toggle display of watts in horizonal bar form.
- ET-Top: Add labels to the display of watts in horizonal bar form.
- ET-Top: Add MISC power at a constant 5.75 watts under the ETSOC power category.
### Changed
### Deprecated
### Removed
### Fixed

## [1.3.0] - 2022-7-8
### Added
### Changed
- ET-Top: Remove -f dump trace stats buffer option and add interactive dump command.
- ET-Top: Add displayOpStats helper and add conversion from milliwatts to watts.
- Rename execute-able from et-top to et-powertop
### Deprecated
### Removed
### Fixed

## [1.2.0] - 2022-7-7
### Added
- ET-Top: Add -f dump trace stats buffer option and version info.
### Changed
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
