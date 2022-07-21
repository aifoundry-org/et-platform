# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]
## [Unrealeased]
### Added
- [SW-12942] implemented a check in host for kernel shire mask
- Added new error code for kernel launches with invalid shire mask
- [SW-11138] added some exceptions catch inside runtime threads which could cause the server to hang.
- [SW-13139] Added getDmaInfo API; which returns a new types. See "runtime/Types.h". The new API call is:  
  ```cpp
    virtual DmaInfo getDmaInfo(DeviceId deviceId) const = 0;  
  ```
### Changed
- [SW-13523] updated DeviceProperties serialization and struct (see "runtime/Types.h") to expose two new fields:
  * tdp
  * form factor
- deviceLayer 0.3.0 version hash pinned.
- Convert from KB to MB the device sizes for L2 and L3 
- Changed gitlab configuration to allow more memory in kubernetes instances
### Deprecated
### Removed
- ***BREAKING CHANGE***: [SW-13511] ***removed*** IDmaBuffers from the runtime API. All three following methods have been removed from the API:
  ```cpp
   virtual EventId memcpyHostToDevice(StreamId stream, const IDmaBuffer* h_src, std::byte* d_dst, size_t size, bool barrier = false)
   virtual EventId memcpyDeviceToHost(StreamId stream, const std::byte* d_src, IDmaBuffer* h_dst, size_t size,bool barrier = true) = 0;
   virtual std::unique_ptr<IDmaBuffer> allocateDmaBuffer(DeviceId device, size_t size, bool writeable) = 0;
  ```

### Fixed
- Adapt device tests to return L2 and L3 sizes in KB
### Security
## [0.3.1]
### Added
- [SW-13062] waitForEvent / waitForStream won't block if timeout argument is 0 seconds
### Changed
### Deprecated
### Removed
### Fixed
### Security
## [0.3.0]
### Added
- [SW-13045] added a new callback which will be executed when a kernel abort happens. See new API function: 
```cpp
virtual void setOnKernelAbortedErrorCallback(const KernelAbortedCallback& callback) = 0;
```
### Changed
### Deprecated
### Removed
### Fixed
### Security
## [0.2.1]
### Added
- [SW-12975] 
  - more tests for memcpy op checks. 
### Changed
  - deviceLayer 0.2.0 version hash pinned.
### Deprecated
### Removed
### Fixed
- [SW-12900]
  - try to merge memcpylists to avoid deadlocks waiting for huge memcpylists
- [SW-12736]
  - fixed memcpy check operation, sometimes it didn't caught some bad memcpy operations (using memory beyond previously reserved).
### Security
## [0.2.0]
### Added
- [SW-12621]
  - added DeviceLayerFake to runtime
- [SW-12315]
  - check device-api compatibility
- this CHANGELOG.md
- [SW-12580]
  - Added CM mask to error handling.
  - StreamError has the new field   
  - ```cpp 
    std::optional<uint64_t> cmShireMask_; /// < only available in some kernel errors. Contains offending shiremask
    ``` 
### Changed
 - *BREAKING CHANGE*: due to check device-api compatibility we need to take care when using IDeviceLayerFake since it doesnt support this command. There is a new option in runtime creation which if set will skip the device-api compatibility check, that needs to be set when using IDeviceLayerFake:

    ```cpp 
    static RuntimePtr create dev::IDeviceLayer* deviceLayer, Options options = getDefaultOptions());
    ```

    Instead of relying on default Options, we need to set to false this one (only when using IDeviceLayerFake): `options.checkDeviceApiVersion_=false`

    Example runtime initialization using IDeviceLayerFake:
    ```cpp
    dev::IDeviceLayerFake fake;
    auto options = rt::getDefaultOptions();
    options.checkDeviceApiVersion_ = false;
    auto runtime = rt::IRuntime::create(&fake, options);
    ```
### Deprecated
### Removed
### Fixed
 - [SW-12696] inconsistency error on CMA memory when running memcpy list DMA operations
### Security


## [0.1.0] -
Initial version; not tracking changes until 0.2.0


