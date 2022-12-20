# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- Added implementation of Trace_Format_String_V() for va_list style calling.
- Added user_profile_events for user code instrumentation
- [SW-5479] Added support to report trace buffer threshold event to host.
### Changed
- Bumped version to 1.0.0.
- Fixed vsnprintf calling.
- Returned the number of bytes written from trace string functions.
- [SW-15435] Embrace SemVer in et-trace decoding (Accept backwards compatible minor)

### Deprecated
### Removed
- Removing deprecated Trace_PMC_Counters_Memory() and ET_TRACE_GET_MEM_SHIRE_COUNTER.
### Fixed
### Security

## [0.9.0] - 2022-12-13
### Added-
- [SW-7460] Added ET_TRACE_VSNPRINTF configurable option and fixed Trace_Format_String() implementation.
### Changed
- [SW-10308] Separate out SC and MS APIs to log PMC events
### Deprecated
### Removed
### Fixed
### Security

## [0.8.0] - 2022-11-22
### Added
### Changed
- [SW-14349] Replaced ET_TRACE_MEM_CPY with ET_TRACE_WRITE_MEM
### Deprecated
### Removed
- [SW-14349] Removed ET_TRACE_MEM_CPY config option for encoder
### Fixed
- Conan build for running tests
### Security

## [0.7.0] - 2022-09-02
### Added
- [SW-13832] Add Trace_Event_Copy API
- Added ET_TRACE_READ_MEM and ET_TRACE_WRITE_MEM macros.
### Changed
- [SW-14083]: Update power status structure to use current power as 16 bits instead of 8
    - This is a breaking change, trace_event_power_status_t now returns 16-bit power value instead of 8-bit
### Deprecated
### Removed
### Fixed
- Read coherency issue in Trace_Event_Copy().
### Security

## [0.6.0] - 2022-8-15
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
- Added resource utilization stats support
- CMake option ENABLE_WARNINGS_AS_ERRORS in tests
### Changed
- Made Trace string length as configurable.
- Adapt conan pipeline to conan 1.49
- Set with_tests to False by default.
### Deprecated
### Removed
### Fixed
- [SW-13631] Aligned trace standard header to cache-line size.
- Build warnings in tests.
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
