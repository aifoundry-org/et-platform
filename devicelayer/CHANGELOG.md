# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]

### Added
### Changed
- Adapted conan CI to conan 1.49+ (Cmake presets)
### Deprecated
### Removed
### Fixed
### Security

## [0.3.0] - 2022-6-25

### Added
### Changed
- [SW-11835] Updated the SP DIRs and device configuration structures.
  * This is a breaking change. The SCP, L2 and L3 sizes are now in KB instead of MB.
  * Packages supported with this change:
    * linuxDriver/0.1.0@#86ada88a12a28a47f52238641b3c9483
    * device-bootloaders/0.2.0@#6563ebf4e4e5db6ee978bf47ca112ac2
### Deprecated
### Removed
### Fixed
- Device Configuration logs.
### Security

## [0.2.0] - 2022-6-10
### Added
- [SW-12683] Added a check for command size in DeviceSysemu
- This changelog
- [SW-12547] Added two new APIs: getDeviceAttribute() and clearDeviceAttributes(), to access ETSoC1 device health and performance statistics through sysfs interface exposed by its Linux Driver.
### Changed
- [SW-12621] as part of IDeviceLayerFake removal, IDeviceLayerMock has also changed. Now its called
DeviceLayerMock (without the I) since its an implementation, not an interface. Also the delegation mechanism has changed, a new constructor receives a pointer to an instance of the IDeviceLayer delegate.
### Deprecated
### Removed
- [SW-12621] removed IDeviceLayerFake from deviceLayer, changed IDeviceLayerMock -> DeviceLayerMock
  * this is a breaking change, see the ticket description.
### Fixed
- [SW-12919] DevicePcie implementation of deviceLayer won't throw if there are no devices
### Security


## [0.1.0] -
Initial version; not tracking changes


