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

## [1.2.0] - 2022-12-07
### Changed
- (Conan) Consume sw-sysemu/0.7.0
- Unconditionally forward CMA allocation request to driver
### Deprecated
- Marked getFreeCmaMemory() as deprecated

## [1.1.0] - 2022-10-20
### Added
- [SW-14409] Added new method reinitDeviceInstance() which facilitates single device reinitialization. Note, the declaration of this interface is non pure virtual in order to not introduce a breaking change. The default implementation will throw an exception.
- [SW-14542] Added a new method hintInactivity which indicates to the device that the host does not expect activity. Currently there is only a sysemu based implementation of this API, which is useful to avoid wasting CPU cycles emulating nothing.
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [1.0.0] - 2022-10-19

### Added
- [SW-14409] Added ETSOC reset flag in sendCommandServiceProcessor(). Now sendCommandServiceProcessor()/sendCommandMasterMinion() APIs use `SPCmdFlag/MMCmdFlag` in place of bool argument `isMmReset`, `isDma` and `isHighPriority` etc.
  * The use of enum `DeviceState` depends on new et_ioctl.h header.
  * Package required for this change:
    * linuxDriver/0.1.0@#9f0abce8d2566d2c178e687df7324c68
- [SW-14538] Add version compatibility check.
### Changed
- *** Breaking change *** [SW-14409] Changed dev::DeviceConfig::archRevision_ from `uint8_t` to `enum dev::DeviceConfig::ArchRevision` to be consistent with `enum dev::DeviceConfig::FormFactor`.
### Deprecated
### Removed
- *** Breaking change *** [SW-14409] Removed old sendCommandServiceProcessor and sendCommandMasterMinion
### Fixed
### Security

## [0.5.0] - 2022-10-06

### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines.
### Changed
- Adapt Conan ci to 1.52.0
- [SW-14434] (Conan) CI now generates Debug & Release pkgs
### Deprecated
### Removed
### Fixed
### Security

## [0.4.0] - 2022-8-15
### Added
### Changed
- Adapted conan CI to conan 1.49+ (Cmake presets)
- [SW-12918] Updated the description for waitForEpollEventsMasterMinion() and waitForEpollEventsServiceProcessor() APIs.
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


