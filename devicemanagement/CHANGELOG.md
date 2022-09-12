# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]

### Added
- [SW-13832] Add new command DM_CMD_GET_MM_STATS to the commandCodeTable
- [SW-13851] Add test for command DM_CMD_SET_DM_STATS_RUN_CONTROL
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.6.0] - 2022-09-05
### Added
- [SW-13954] Added setAndGetModulePartNumber
- [SW-13954] Added setAndGetModuleVoltage
- Ranges for conan packages version requirements.
### Changed
- [SW-13850] Disabling setThrottlePowerStatus test on silicon until SW-13953 is resolved
- [SW-13850] Disabling setAndGetModuleFrequency test until SW-13952 is resolved
- [SW-13953] Removing workarrounds for trace related tests and fixed SP trace log levels
- [SW-13954] updated getFirmwareRevisions test to also print pmic_v
- [SW-14044] Updating power value to remove hex conversion
- Moved setAndGetModuleFrequency test to Ops node dependent category of tests.
- [SW-14115] Converting module power from 10mW to W.
### Deprecated
### Removed
### Fixed
- [SW-13978] Enabling back setAndGetModuleFrequency
- Fixed setTraceControl test and fixed a log.
### Security

## [0.5.0] - 2022-08-15
### Added
- [SW-6111] Add basic info to Device Management Overview section.

### Changed
- Updated `setModuleFrequency` test to revert back to original frequencies and
  check by setting a list of test frequencies.
- Renamed `setModuleFrequency` to `setAndGetModuleFrequency` because now this
  test sets the frequency and validates it by reading back those frequencies.
- [SW-13583] Enabled getFWBootstatus test
- [SW-13620] Add new command DM_CMD_GET_SP_STATS to the commandCodeTable
- Show output on failure for mix tests.
- Disabled getFWBootstatus test. To be fixed under SW-13807.
### Deprecated
### Removed
### Fixed
- Conanfile .libs specification
### Security

## [0.4.0] - 2022-07-07
### Added
### Changed
- Disabled `getModuleVoltage` test on Target::Silicon. The test runs fine in the
  beginning but in longer runs of ops + mgmt regression, it times out.
- Compare received speed with either GEN3 or GEN4 speed in test
  `getModulePCIEPortsMaxSpeed`. This will be revert back to comparing the speed
  with max_link_speed sysfs attribute once fixed in SW-13272.
- Adding a delay in getSpTraceBuffer test as a temporary workaround.
- Disable getModulePower test on SysEMU.
- `setThrottlePowerStatus` Removed trace enable/disable within test.
-  Updated test `getModuleSerialNumber`. Enabled test on silicon
- [SW-13282] Enabled `getModulePower` and `getModuleVoltage` on silicon and SysEMU.
### Deprecated
### Removed
### Fixed
- [SW-13282] Skipped validation in `getModulePower` and `getModuleVoltage` for SysEMU and Loopback.
### Security

## [0.3.0] - 2022-07-4
### Added
- Added .clang-format file based on LLVM style.
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines.
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
- Disabled getModulePCIEPortsMaxSpeed test on SysEMU.
- Disabled setPCIELinkSpeed test on Silicon. To be enabled back in SW-13272
- Disable active power management before exiting test setModuleActivePowerManagement.
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
- Include .cc file types scan in clang CI job.
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
