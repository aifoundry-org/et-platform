# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
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


