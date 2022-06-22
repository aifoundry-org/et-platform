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

## [0.5.0] - 2022-6-22
### Added
### Changed
-- [SW-12373] - Removing compatibility checks on device side
-- [SW-12598] - Moved project versioning to cmake.

### Deprecated
### Removed
-- [SW-12598] - YAML based project versioning removed.
### Fixed
### Security


## [0.4.0]
### Added
### Changed
-- [SW-12007] Added CM shire mask in kernel launch response.
-- [SW-11969] use field to hold firmware release revision
-- [SW-11723]: Updating payload field size to 64 bit

### Deprecated
### Removed
### Fixed
### Security

## [0.3.0]
### Added
### Changed
-- [SW-11460-11572] Get temperature update and set frequency command added
### Deprecated
### Removed
### Fixed
### Security

## [0.2.0]
### Added
### Changed
-- [SW-10736]: Renaming CM reset to MM reset
-- [SW-11105] fix trace configure event enum values
-- SW-11009 - Fix bug in encoding
-- Updated YAML to add cmds for MDI mem write
-- [sw-10540] Added new error codes and comments for deprecated error codes.
-- [SW-10770] Updated CM reset command
-- Updates to management-api.yaml to support GDB debug command using MDI
-- [SW-10770] Adding new Event enum values for Asycn Events
-- [SW-10770] Adding support to reset CM from Host Runtime
-- SW-10348 Adding DM service support to reset the MM
-- [SW-10618] device_ops_kernel_launch_rsp: include error pointers
-- [SW-10136]: Adding API to change throttle power states
-- [SW-8942]: Adding error aborted response in DMA commands.
-- [SW-8942]: Adding enum entry for DMA unknown error.
-- Add Kernel Command Flags for embedding kernel args
-- SW-9243 Correcting the RT control trace uart enable flag

### Deprecated
### Removed
### Fixed
### Security


## [0.1.0] -
Initial version

[Unreleased]: https://gitlab.esperanto.ai/software/device-api/-/compare/v0.4.0...master
[0.4.0]: https://gitlab.esperanto.ai/software/device-api/-/tags/v0.4.0..v0.3.0
[0.3.0]: https://gitlab.esperanto.ai/software/device-api/-/tags/v0.3.0..v0.2.0
[0.2.0]: https://gitlab.esperanto.ai/software/device-api/-/tags/v0.2.0..v0.1.0
