# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
### Changed
- `getModuleFWRevision` test now prints out extracted firmware versions instead
  of comparing with hard-coded versions. Included package fmt for formatting of
  these prints.
- `getDeviceErrorEvents` test allows passing of test with warning message if
  dmesg is inaccessible.
- Disable only the un-supported tests for Target::Silicon instead of all tests
  of that management service.
- [SW-13218] Re-enabling the getModuleMemorySizeMB and getModuleMemoryVendorPartNumber tests for silicon.
- `getModulePCIENumPortsMaxSpeed` test renamed to `getModulePCIEPortsMaxSpeed`.
### Deprecated
### Removed
- Workaround for SP Trace
### Fixed
- `getDeviceErrorEvents` test now reads error events information from sysfs
  counters instead of reading it from dmesg. Enabling this test back for
  Target::Silicon.
- `getModulePCIEPortsMaxSpeed` test now compare the received max link speed with
  sysfs attribute file `max_link_speed`. Enabling this test back for
  Target::Silicon.
### Security

## [0.2.0] - 2022-06-24
### Added
- Added new test for MM stat buffer under Ops node dependant tests category.
- Added a local copy of trace buffer data.
### Changed
### Deprecated
### Removed
### Fixed
- Catching exception from waitForEpollEventsServiceProcessor().
### Security

## [0.1.2] - 2022-06-20
### Added
- This CHANGELOG file.
### Changed
### Deprecated
### Removed
### Fixed
### Security
