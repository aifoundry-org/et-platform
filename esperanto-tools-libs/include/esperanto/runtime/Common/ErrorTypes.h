//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ERROR_TYPES_H_
#define ERROR_TYPES_H_

/// @file

/**
 * @enum etrtError
 * @brief ET Runtime error types
 *
 * Most calls to the ET Runtime API return a value of this type.
 */
enum etrtError {
  /**
   * The API call returned with no errors. In the case of query calls, this
   * can also mean that the operation being queried is complete (see
   * ::etrtEventQuery() and ::etrtStreamQuery()).
   */
  etrtSuccess = 0,

  /**
   * The device function being invoked (usually via ::etrtLaunchKernel()) was
   * not previously configured via the ::etrtConfigureCall() function.
   */
  etrtErrorMissingConfiguration = 1,

  /**
   * The API call failed because it was unable to allocate enough memory to
   * perform the requested operation.
   */
  etrtErrorMemoryAllocation = 2,

  /**
   * The API call failed because the ET driver and runtime could not be
   * initialized.
   */
  etrtErrorInitializationError = 3,

  /**
   * An exception occurred on the device while executing a kernel. Common
   * causes include dereferencing an invalid device pointer and accessing
   * out of bounds shared memory. The device cannot be used until
   * ::etrtThreadExit() is called. All existing device memory allocations
   * are invalid and must be reconstructed if the program is to continue
   * using the ET software stack.
   */
  etrtErrorLaunchFailure = 4,

  /**
   * This indicates that the device kernel took too long to execute. This can
   * only occur if timeouts are enabled - see the device property
   * \ref ::etrtDeviceProp::kernelExecTimeoutEnabled "kernelExecTimeoutEnabled"
   * for more information.
   * This leaves the process in an inconsistent state and any further ET Runtime
   * work will return the same error. To continue using the ET stack, the
   * process must be terminated and relaunched.
   */
  etrtErrorLaunchTimeout = 6,

  /**
   * This indicates that a launch did not occur because it did not have
   * appropriate resources. Although this error is similar to
   * ::etrtErrorInvalidConfiguration, this error usually indicates that the
   * user has attempted to pass too many arguments to the device kernel, or the
   * kernel launch specifies too many threads for the kernel's register count.
   */
  etrtErrorLaunchOutOfResources = 7,

  /**
   * The requested device function does not exist or is not compiled for the
   * proper device architecture.
   */
  etrtErrorInvalidDeviceFunction = 8,

  /**
   * This indicates that a kernel launch is requesting resources that can
   * never be satisfied by the current device. Requesting more shared memory
   * per block than the device supports will trigger this error, as will
   * requesting too many threads or blocks. See ::etrtDeviceProp for more
   * device limitations.
   */
  etrtErrorInvalidConfiguration = 9,

  /**
   * This indicates that the device ordinal supplied by the user does not
   * correspond to a valid ET device.
   */
  etrtErrorInvalidDevice = 10,

  /**
   * This indicates that one or more of the parameters passed to the API call
   * is not within an acceptable range of values.
   */
  etrtErrorInvalidValue = 11,

  /**
   * This indicates that one or more of the pitch-related parameters passed
   * to the API call is not within the acceptable range for pitch.
   */
  etrtErrorInvalidPitchValue = 12,

  /**
   * This indicates that the symbol name/identifier passed to the API call
   * is not a valid name or identifier.
   */
  etrtErrorInvalidSymbol = 13,

  /**
   * This indicates that the buffer object could not be mapped.
   */
  etrtErrorMapBufferObjectFailed = 14,

  /**
   * This indicates that the buffer object could not be unmapped.
   */
  etrtErrorUnmapBufferObjectFailed = 15,

  /**
   * This indicates that at least one host pointer passed to the API call is
   * not a valid host pointer.
   */
  etrtErrorInvalidHostPointer = 16,

  /**
   * This indicates that at least one device pointer passed to the API call is
   * not a valid device pointer.
   */
  etrtErrorInvalidDevicePointer = 17,

  /**
   * This indicates that the texture passed to the API call is not a valid
   * texture.
   */
  etrtErrorInvalidTexture = 18,

  /**
   * This indicates that the texture binding is not valid. This occurs if you
   * call ::etrtGetTextureAlignmentOffset() with an unbound texture.
   */
  etrtErrorInvalidTextureBinding = 19,

  /**
   * This indicates that the channel descriptor passed to the API call is not
   * valid. This occurs if the format is not one of the formats specified by
   * ::etrtChannelFormatKind, or if one of the dimensions is invalid.
   */
  etrtErrorInvalidChannelDescriptor = 20,

  /**
   * This indicates that the direction of the memcpy passed to the API call is
   * not one of the types specified by ::etrtMemcpyKind.
   */
  etrtErrorInvalidMemcpyDirection = 21,

  /**
   * This indicates that a ET Runtime API call cannot be executed because
   * it is being called during process shut down, at a point in time after
   * ET driver has been unloaded.
   */
  etrtErrorEtrtUnloading = 29,

  /**
   * This indicates that an unknown internal error has occurred.
   */
  etrtErrorUnknown = 30,

  /**
   * This indicates that a resource handle passed to the API call was not
   * valid. Resource handles are opaque types like ::Stream * and
   * ::Event *.
   */
  etrtErrorInvalidResourceHandle = 33,

  /**
   * This indicates that asynchronous operations issued previously have not
   * completed yet. This result is not actually an error, but must be indicated
   * differently than ::etrtSuccess (which indicates completion). Calls that
   * may return this value include ::etrtEventQuery() and ::etrtStreamQuery().
   */
  etrtErrorNotReady = 34,

  /**
   * This indicates that the installed ET driver is older than the ET Runtime
   * This is not a supported configuration. Users should install an updated  ET
   * driver to allow the application to run.
   */
  etrtErrorInsufficientDriver = 35,

  /**
   * This indicates that the user has called ::etrtSetValidDevices(),
   * ::etrtSetDeviceFlags() after initializing the ET Runtime by
   * calling non-device management operations (allocating memory and
   * launching kernels are examples of non-device management operations).
   * This error can also be returned if using runtime/driver
   * interoperability and there is an existing context active on the
   * host thread.
   */
  etrtErrorSetOnActiveProcess = 36,

  /**
   * This indicates that the surface passed to the API call is not a valid
   * surface.
   */
  etrtErrorInvalidSurface = 37,

  /**
   * This indicates that no ET devices were detected by the installed Driver.
   */
  etrtErrorNoDevice = 38,

  /**
   * This indicates that an uncorrectable ECC error was detected during
   * execution.
   */
  etrtErrorECCUncorrectable = 39,

  /**
   * This indicates that a link to a shared object failed to resolve.
   */
  etrtErrorSharedObjectSymbolNotFound = 40,

  /**
   * This indicates that initialization of a shared object failed.
   */
  etrtErrorSharedObjectInitFailed = 41,

  /**
   * This indicates that the ::etrtLimit passed to the API call is not
   * supported by the active device.
   */
  etrtErrorUnsupportedLimit = 42,

  /**
   * This indicates that multiple global or constant variables (across separate
   * CUDA source files in the application) share the same string name.
   */
  etrtErrorDuplicateVariableName = 43,

  /**
   * This indicates that multiple textures (across separate CUDA source
   * files in the application) share the same string name.
   */
  etrtErrorDuplicateTextureName = 44,

  /**
   * This indicates that multiple surfaces (across separate CUDA source
   * files in the application) share the same string name.
   */
  etrtErrorDuplicateSurfaceName = 45,

  /**
   * This indicates that all ET devices are busy or unavailable at the current
   * time. Devices are often busy/unavailable due to use of
   * ::etrtComputeModeExclusive, ::etrtComputeModeProhibited or when long
   * running kernels have filled up the device and are blocking new work
   * from starting. They can also be unavailable due to memory constraints
   * on a device that already has active ET Runtime work being performed.
   */
  etrtErrorDevicesUnavailable = 46,

  /**
   * This indicates that the device kernel image is invalid.
   */
  etrtErrorInvalidKernelImage = 47,

  /**
   * This indicates that there is no kernel image available that is suitable
   * for the device. This can occur when a user specifies code generation
   * options for a particular CUDA source file that do not include the
   * corresponding device configuration.
   */
  etrtErrorNoKernelImageForDevice = 48,

  /**
   * This indicates that the current context is not compatible with this
   * the ET Runtime.
   */
  etrtErrorIncompatibleDriverContext = 49,

  /**
   * This error indicates that a call to ::etrtDeviceEnablePeerAccess() is
   * trying to re-enable peer addressing on from a context which has already
   * had peer addressing enabled.
   */
  etrtErrorPeerAccessAlreadyEnabled = 50,

  /**
   * This error indicates that ::etrtDeviceDisablePeerAccess() is trying to
   * disable peer addressing which has not been enabled yet via
   * ::etrtDeviceEnablePeerAccess().
   */
  etrtErrorPeerAccessNotEnabled = 51,

  /**
   * This indicates that a call tried to access an exclusive-thread device that
   * is already in use by a different thread.
   */
  etrtErrorDeviceAlreadyInUse = 54,

  /**
   * This indicates profiler is not initialized for this run. This can
   * happen when the application is running with external profiling tools
   * like visual profiler.
   */
  etrtErrorProfilerDisabled = 55,

  /**
   * An assert triggered in device code during kernel execution. The device
   * cannot be used again until ::etrtThreadExit() is called. All existing
   * allocations are invalid and must be reconstructed if the program is to
   * continue using the ET stack.
   */
  etrtErrorAssert = 59,

  /**
   * This error indicates that the hardware resources required to enable
   * peer access have been exhausted for one or more of the devices
   * passed to ::etrtEnablePeerAccess().
   */
  etrtErrorTooManyPeers = 60,

  /**
   * This error indicates that the memory range passed to ::etrtHostRegister()
   * has already been registered.
   */
  etrtErrorHostMemoryAlreadyRegistered = 61,

  /**
   * This error indicates that the pointer passed to ::etrtHostUnregister()
   * does not correspond to any currently registered memory region.
   */
  etrtErrorHostMemoryNotRegistered = 62,

  /**
   * This error indicates that an OS call failed.
   */
  etrtErrorOperatingSystem = 63,

  /**
   * This error indicates that P2P access is not supported across the given
   * devices.
   */
  etrtErrorPeerAccessUnsupported = 64,

  /**
   * This error indicates that a device runtime grid launch did not occur
   * because the depth of the child grid would exceed the maximum supported
   * number of nested grid launches.
   */
  etrtErrorLaunchMaxDepthExceeded = 65,

  /**
   * This error indicates that a grid launch did not occur because the kernel
   * uses file-scoped textures which are unsupported by the device runtime.
   * Kernels launched via the device runtime only support textures created with
   * the Texture Object API's.
   */
  etrtErrorLaunchFileScopedTex = 66,

  /**
   * This error indicates that a grid launch did not occur because the kernel
   * uses file-scoped surfaces which are unsupported by the device runtime.
   * Kernels launched via the device runtime only support surfaces created with
   * the Surface Object API's.
   */
  etrtErrorLaunchFileScopedSurf = 67,

  /**
   * This error indicates that a call to ::etrtDeviceSynchronize made from
   * the device runtime failed because the call was made at grid depth greater
   * than than either the default (2 levels of grids) or user specified device
   * limit ::etrtLimitDevRuntimeSyncDepth. To be able to synchronize on
   * launched grids at a greater depth successfully, the maximum nested
   * depth at which ::etrtDeviceSynchronize will be called must be specified
   * with the ::etrtLimitDevRuntimeSyncDepth limit to the ::etrtDeviceSetLimit
   * api before the host-side launch of a kernel using the device runtime.
   * Keep in mind that additional levels of sync depth require the runtime
   * to reserve large amounts of device memory that cannot be used for
   * user allocations.
   */
  etrtErrorSyncDepthExceeded = 68,

  /**
   * This error indicates that a device runtime grid launch failed because
   * the launch would exceed the limit ::etrtLimitDevRuntimePendingLaunchCount.
   * For this launch to proceed successfully, ::etrtDeviceSetLimit must be
   * called to set the ::etrtLimitDevRuntimePendingLaunchCount to be higher
   * than the upper bound of outstanding launches that can be issued to the
   * device runtime. Keep in mind that raising the limit of pending device
   * runtime launches will require the runtime to reserve device memory that
   * cannot be used for user allocations.
   */
  etrtErrorLaunchPendingCountExceeded = 69,

  /**
   * This error indicates the attempted operation is not permitted.
   */
  etrtErrorNotPermitted = 70,

  /**
   * This error indicates the attempted operation is not supported
   * on the current system or device.
   */
  etrtErrorNotSupported = 71,

  /**
   * Device encountered an error in the call stack during kernel execution,
   * possibly due to stack corruption or exceeding the stack size limit.
   * This leaves the process in an inconsistent state and any further ET Runtime
   * work will return the same error. To continue using the ET stack, the
   * process must be terminated and relaunched.
   */
  etrtErrorHardwareStackError = 72,

  /**
   * The device encountered an illegal instruction during kernel execution
   * This leaves the process in an inconsistent state and any further ET Runtime
   * work will return the same error. To continue using the ET stack, the
   * process must be terminated and relaunched.
   */
  etrtErrorIllegalInstruction = 73,

  /**
   * The device encountered a load or store instruction
   * on a memory address which is not aligned.
   * This leaves the process in an inconsistent state and any further ET Runtime
   * work will return the same error. To continue using the ET stack, the
   * process must be terminated and relaunched.
   */
  etrtErrorMisalignedAddress = 74,

  /**
   * While executing a kernel, the device encountered an instruction
   * which can only operate on memory locations in certain address spaces
   * (global, shared, or local), but was supplied a memory address not
   * belonging to an allowed address space.
   * This leaves the process in an inconsistent state and any further ET Runtime
   * work will return the same error. To continue using the ET stack, the
   * process must be terminated and relaunched.
   */
  etrtErrorInvalidAddressSpace = 75,

  /**
   * The device encountered an invalid program counter.
   * This leaves the process in an inconsistent state and any further ET Runtime
   * work will return the same error. To continue using the ET stack, the
   * process must be terminated and relaunched.
   */
  etrtErrorInvalidPc = 76,

  /**
   * The device encountered a load or store instruction on an invalid memory
   * address. This leaves the process in an inconsistent state and any further
   * ET Runtime work will return the same error. To continue using the ET stack,
   * the process must be terminated and relaunched.
   */
  etrtErrorIllegalAddress = 77,

  /**
   * A PTX compilation failed. The runtime may fall back to compiling PTX if
   * an application does not contain a suitable binary for the current device.
   */
  etrtErrorInvalidPtx = 78,

  /**
   * This indicates an error with the OpenGL or DirectX context.
   */
  etrtErrorInvalidGraphicsContext = 79,

  /**
   * This indicates that an uncorrectable NVLink error was detected during the
   * execution.
   */
  etrtErrorNvlinkUncorrectable = 80,

  /**
   * This indicates that the PTX JIT compiler library was not found. The JIT
   * Compiler library is used for PTX compilation. The runtime may fall back to
   * compiling PTX if an application does not contain a suitable binary for the
   * current device.
   */
  etrtErrorJitCompilerNotFound = 81,

  /**
   * This error indicates that the number of blocks launched per grid for a
   * kernel that was launched via either ::etrtLaunchCooperativeKernel or
   * ::etrtLaunchCooperativeKernelMultiDevice exceeds the maximum number of
   * blocks as allowed by ::etrtOccupancyMaxActiveBlocksPerMultiprocessor or
   * ::etrtOccupancyMaxActiveBlocksPerMultiprocessorWithFlags times the number
   * of multiprocessors as specified by the device attribute
   * ::etrtDevAttrMultiProcessorCount.
   */
  etrtErrorCooperativeLaunchTooLarge = 82,

  etrtErrorRuntime = 83,

  /**
   * This error indicates that we encountered an error during device
   * configuration
   */
  etrtErrorDeviceConfig,

  /**
   * This indicates that the ELF data has already been read
   */
  etrtErrorModuleELFDataExists,

  /**
   * This error indicates that the module is not loaded on the device
   */
  etrtErrorModuleNotOnDevice,

  /**
   * This error indicates that we failed to load a module on the device
   */
  etrtErrorModuleFailedToLoadOnDevice,

  /**
   * This error indicates that we failed destroy a code module
   */
  etrtErrorModuleFailedToDestroy,

  /**
   * This error indicates that we failed to deallocate memory, could not find
   * the allocated tensor
   */
  etrtErrorFreeUnknownTensor,

  /**
   * This indicates an internal startup failure in the ET Runtime.
   */
  etrtErrorStartupFailure = 0x7f,

  etrtMailBoxWriteError,
};

#endif // ERROR_TYPES_H_
