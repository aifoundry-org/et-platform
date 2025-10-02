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

## [0.4.0]
### Added
### Changed
- (Conan) Enable building project as editable
- (CMake) Honor CMAKE_<LANG>_VISIBILITY_PRESET and handle exporting only public symbols
- (Conan) Support 'fvisibility' option to be able to restrict symbol visibility on demand.
  For now default symbol visibility is kept as 'default'.
- (CI) Run smoke/build jobs with "et-host-utils:fvisibility=hidden" to catch symbol visibility issues early
### Deprecated
### Removed
### Fixed
### Security

## [0.3.0]
### Added
- [SW-16456] added a basic actionList. Also added a simple Runner which will use a worker thread to run the actionList.

## [0.2.0]
### Added
- [SW-15414] added blockUntilDrained method (blocks until all tasks have finished) and parameter to drain at destruction.
### Changed
- (CMake) Do not use non-standard g3log::g3log target. Use target defined upstream (g3log) instead.
### Deprecated
### Removed
### Fixed
### Security


## [0.1.0] - 
Initial version; not tracking changes


