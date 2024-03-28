# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- [CS-226] Add support for FRU commands 
- Add GET/SET FRU tests
### Changed
- Initialize FRU test data
### Deprecated
### Removed
- Removing UpdateFirmware to reduce execution time, improve survivability of Flash
### Fixed
- Fix type casting in utest
- Fix tests/TestDevMgmtApiSyncCmds.cpp gcc11 compatibility
### Security

## [0.16.0] - 2024-01-30
### Added
- [SW-19638] Add control option to dump bin traces in files
### Changed
- [SW-19638] Disabled dump of raw traces to bin files
- [SW-20169] Double fw update test timeout for multiple devices 
### Deprecated
### Removed
### Fixed
### Security

## [0.15.0] - 2023-12-15
### Added
- [SW-19470] Added memory sanitizers support
- [SW-18613] Added support to change the PMIC FW metadata in the flash image and re-generate it for testing.
- Adding updateFirmwareImageAndResetMultiDevice test.
- Generate packages with linux-ubuntu22.04-x86_64-gcc11-release
### Changed
- Disabling setModuleActivePowerManagement since the DVFS is not stable.
- [SW-18613] Change the test to modify PMIC FW hash instead of PMIC FW version.
- (Conan) Use Conan 2.0 imports
### Deprecated
### Removed
### Fixed
- [SW-19324] Fixed nullptr dereference issue in dumpRawTraceBuffer function causing exception when SP trace buffer full event is processed
- [SW-19698] Extended library to support FMTlib V10
### Security

## [0.14.0] - 2023-11-07
### Added
- [SW-18590] Added firmware update functional and stress tests.
- [SW-15748] Handling DM events in userspace
  - Adding DM event processing thread for each device
### Changed
- [SW-18398b] Update DevErrorEvent implementation for the skipList usage to add/remove EventType entry from the default skipList without requiring to replace the complete skipList.
[SW-18797] Improvements for event processor
  * Do not run event processor during ETSOC reset
  * Run the thread in detach mode to exit immediately when test completes
  * Add {init|cleanup}DMTestFramework() calls to reduce code duplication
### Deprecated
- [SW-15748] Marked getEvent() API as deprecated, will be replaced with getMDIEvents()
### Removed
### Fixed
- [SW-18926] Fixed trace filter mask in DM tests
### Security

## [0.13.0] - 2023-09-01
### Added
- [SW-18352] Add support to check device error events
### Changed
- [SW-18398] Restrict check for device error events
### Deprecated
### Removed
### Fixed
- [SW-18352b] Fix check device error events logic when isCheckList set to false
### Security

## [0.12.0] - 2023-08-03
### Added
### Changed
- Adapt CI to gitlab-ci v15
- [SW-17593] update conan dependency for deviceApi
- Updating the conan dependencies versions to released components
### Deprecated
### Removed
### Fixed
- [CI] Adapt pipeline and conanfile.py to gitlab.com
- [SW-17936] Fixed Temperature threshold test case to restore threshold value at the end
### Security

## [0.11.0] - 2023-06-02
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Increase resetSOC tests timeout for multi-card (8) machines
- [SW-17936] Fixed TDP test case to restore TDP value at the end
### Security

## [0.10.0] - 2023-04-19
### Added
### Changed
- [SW-16558] Updated setPCIELaneWidth test to restore the default lane width
- [SW-16538] Disabled setAndGetModuleVoltage test
### Deprecated
### Removed
### Fixed
- [SW-16851] Save trace logs when service request times out
  * Updated the service request timeout to be relatively less than the test timeout to ensure teardown path always executes
  * Change service request return code checks from EXPECT to ASSERT to immediately get out of test body
### Security

## [0.9.0] - 2023-03-15
### Added
- [SW-16234] Added test to verify shire cache get/set configuration command
  - Modify Shire cache configuration test to use scp, l2 and l3 sizes in MB
### Changed
- (Conan) Generate full SemVer versions only when tagging the project. Generate pre-release otherwise.
- (CI) Enable tags pipelines and filter out MR "development/testing" jobs from release pipeline
- [SW-16237] Enabled MDI commands in release mode and fixed MDI tests
- [SW-16237] Added MDI privileged region access validation tests
### Deprecated
### Removed
- (Conan) Remove setting CMAKE_MODULE_PATH with cmake-modules
### Fixed
- [SW-15389] Fixed MDI tests
### Security

## [0.8.0] - 2023-01-10
### Added
- Updating the test plan for resetSOC tests.
- Added NOPARALLEL label for tests which should not be run in parallel regression.
- [SW-15160] Added resetSOC stress tests.
- [SW-15161] Added resetSOCWithOpsInUse test
### Changed
- [SW-14288] Enabled back setAndGetModuleVoltage test.
- Updated deviceApi version in Conan requirements.
- Replaced NOPARALLEL label with PARALLEL env variable. The setAndGetModuleVoltage test will be skipped when PARALLEL env is set.
### Deprecated
### Removed
- Removed device-fw-test-plan.ods.
- [SW-14210] Fixed bug in setThrottlePowerStatus test
### Fixed
- [SW-15493] Fixed getModuleMemorySizeMB test to expect multiple values
### Security

## [0.7.0] - 2022-10-21
### Added
- [SW-13832] Add new command DM_CMD_GET_MM_STATS to the commandCodeTable
- [SW-13851] Add test for command DM_CMD_SET_DM_STATS_RUN_CONTROL
- [SW-14061] Added DM_CMD_GET_ASIC_VOLTAGE test.
- [SW-14337] Add ETSOC reset handling and its test case.
- [SW-14409] Add ETSOC reset per device handling.
### Changed
- [SW-14061] Enabled back setAndGetModuleVoltage test.
- [SW-14288] Disabled the test until SW-14288 is resolved.
- [SW-14288] Changing voltage percentage drop to zero in setAndGetModuleVoltage test.
- [FV-317] Updating asic_voltage elements to be 16bits
### Deprecated
### Removed
### Fixed
- [SW-14061] Fixing the convertion of binary to mV and mV to binary voltage values.
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
