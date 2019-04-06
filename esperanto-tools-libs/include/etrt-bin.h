/**
 * Copyright (C) 2018, Esperanto Technologies Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file etrt-bin.h
 * @brief ET Runtime Library Includes
 * @version 0.1.0
 *
 * @ingroup ETCRT_API
 *
 * ????
 **/

#ifndef ETRT_BIN_H
#define ETRT_BIN_H

#include <stddef.h>

#define etrtHostAllocDefault 0x00 /**< Default page-locked allocation flag */
#define etrtHostAllocPortable                                                  \
  0x01 /**< Pinned memory accessible by all contexts */
#define etrtHostAllocMapped 0x02        /**< Map allocation into device space */
#define etrtHostAllocWriteCombined 0x04 /**< Write-combined memory */

#define etrtHostRegisterDefault                                                \
  0x00 /**< Default host memory registration flag */
#define etrtHostRegisterPortable                                               \
  0x01 /**< Pinned memory accessible by all contexts */
#define etrtHostRegisterMapped                                                 \
  0x02 /**< Map registered memory into device space */
#define etrtHostRegisterIoMemory 0x04 /**< Memory-mapped I/O space */

#define etrtPeerAccessDefault 0x00 /**< Default peer addressing enable flag */

#define etrtStreamDefault 0x00 /**< Default stream flag */
#define etrtStreamNonBlocking                                                  \
  0x01 /**< Stream does not synchronize with stream 0 (the NULL stream) */

/**
 * @brief Per-thread stream handle
 *
 * Stream handle that can be passed as a etrtStream_t to use an implicit stream
 * with per-thread synchronization behavior.
 *
 * See details of the @link_sync_behavior
 */
#define etrtStreamPerThread ((etrtStream_t)0x2)

#define etrtEventDefault 0x00       /**< Default event flag */
#define etrtEventBlockingSync 0x01  /**< Event uses blocking synchronization */
#define etrtEventDisableTiming 0x02 /**< Event will not record timing data */
#define etrtEventInterprocess                                                  \
  0x04 /**< Event is suitable for interprocess use. etrtEventDisableTiming     \
          must be set */

#define etrtDeviceScheduleAuto 0x00 /**< Device flag - Automatic scheduling */
#define etrtDeviceScheduleSpin                                                 \
  0x01 /**< Device flag - Spin default scheduling */
#define etrtDeviceScheduleYield                                                \
  0x02 /**< Device flag - Yield default scheduling */
#define etrtDeviceScheduleBlockingSync                                         \
  0x04 /**< Device flag - Use blocking synchronization */
#define etrtDeviceScheduleMask 0x07 /**< Device schedule flags mask */
#define etrtDeviceMapHost                                                      \
  0x08 /**< Device flag - Support mapped pinned allocations */
#define etrtDeviceLmemResizeToMax                                              \
  0x10 /**< Device flag - Keep local memory allocation after launch */
#define etrtDeviceMask 0x1f /**< Device flags mask */

#define etrtIpcMemLazyEnablePeerAccess                                         \
  0x01 /**< Automatically enable peer access between remote devices as needed  \
        */

#define etrtMemAttachGlobal                                                    \
  0x01 /**< Memory can be accessed by any stream on any device*/
#define etrtMemAttachHost                                                      \
  0x02 /**< Memory cannot be accessed by any stream on any device */
#define etrtMemAttachSingle                                                    \
  0x04 /**< Memory can only be accessed by a single stream on the associated   \
          device */

#define etrtOccupancyDefault 0x00 /**< Default behavior */
#define etrtOccupancyDisableCachingOverride                                    \
  0x01 /**< Assume global caching is enabled and cannot be automatically       \
          turned off */

#define etrtCpuDeviceId ((int)-1) /**< Device id that represents the CPU */
#define etrtInvalidDeviceId                                                    \
  ((int)-2) /**< Device id that represents an invalid device */

/**
 * @brief  Flag to make kernel launch obey pre-synch rules
 *
 * If set, each kernel launched as part of
 * ::etrtLaunchCooperativeKernelMultiDevice only waits for prior work in the
 * stream corresponding to that Device to complete before the kernel begins
 * execution.
 */
#define etrtCooperativeLaunchMultiDeviceNoPreSync 0x01

/**
 * @brief  Flag to make kernel launch obey post-synch rules
 *
 * If set, any subsequent work pushed in a stream that participated in a call to
 * ::etrtLaunchCooperativeKernelMultiDevice will only wait for the kernel
 * launched on the Device corresponding to that stream to complete before it
 * begins execution.
 */
#define etrtCooperativeLaunchMultiDeviceNoPostSync 0x02

/**
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
   * valid. Resource handles are opaque types like ::etrtStream_t and
   * ::etrtEvent_t.
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

  /**
   * This indicates an internal startup failure in the ET Runtime.
   */
  etrtErrorStartupFailure = 0x7f,
};

/**
 * @brief ET Runtime memory types
 */
enum etrtMemoryType {
  etrtMemoryTypeHost = 1,  /**< Host memory */
  etrtMemoryTypeDevice = 2 /**< Device memory */
};

/**
 * @brief ET Runtime memory copy types
 */
enum etrtMemcpyKind {
  etrtMemcpyHostToHost = 0,     /**< Host   -> Host */
  etrtMemcpyHostToDevice = 1,   /**< Host   -> Device */
  etrtMemcpyDeviceToHost = 2,   /**< Device -> Host */
  etrtMemcpyDeviceToDevice = 3, /**< Device -> Device */
  etrtMemcpyDefault =
      4 /**< Direction of the transfer is inferred from the pointer values.
           Requires unified virtual addressing */
};

/**
 * @brief ET Runtime pointer attributes
 */
struct etrtPointerAttributes {
  /**
   * The physical location of the memory, ::etrtMemoryTypeHost or
   * ::etrtMemoryTypeDevice.
   */
  enum etrtMemoryType memoryType;

  /**
   * The device against which the memory was allocated or registered.
   * If the memory type is ::etrtMemoryTypeDevice then this identifies
   * the device on which the memory referred physically resides.  If
   * the memory type is ::etrtMemoryTypeHost then this identifies the
   * device which was current when the memory was allocated or registered
   * (and if that device is deinitialized then this allocation will vanish
   * with that device's state).
   */
  int device;

  /**
   * The address which may be dereferenced on the current device to access
   * the memory or NULL if no such address exists.
   */
  void *devicePointer;

  /**
   * The address which may be dereferenced on the host to access the
   * memory or NULL if no such address exists.
   */
  void *hostPointer;

  /**
   * Indicates if this pointer points to managed memory
   */
  int isManaged;
};

/**
 * @brief ET Device properties
 */
struct etrtDeviceProp {
  char name[256];           /**< ASCII string identifying device */
  size_t totalGlobalMem;    /**< Global memory available on device in bytes */
  size_t sharedMemPerBlock; /**< Shared memory available per block in bytes */
  int regsPerBlock;         /**< 32-bit registers available per block */
  int warpSize;             /**< Warp size in threads */
  size_t memPitch; /**< Maximum pitch in bytes allowed by memory copies */
  int maxThreadsPerBlock;  /**< Maximum number of threads per block */
  int maxThreadsDim[3];    /**< Maximum size of each dimension of a block */
  int maxGridSize[3];      /**< Maximum size of each dimension of a grid */
  int clockRate;           /**< Clock frequency in kilohertz */
  size_t totalConstMem;    /**< Constant memory available on device in bytes */
  int major;               /**< Major compute capability */
  int minor;               /**< Minor compute capability */
  size_t textureAlignment; /**< Alignment requirement for textures */
  size_t texturePitchAlignment; /**< Pitch alignment requirement for texture
                                   references bound to pitched memory */
  int multiProcessorCount;      /**< Number of multiprocessors on device */
  int kernelExecTimeoutEnabled; /**< Specified whether there is a run time limit
                                   on kernels */
  int integrated;            /**< Device is integrated as opposed to discrete */
  int canMapHostMemory;      /**< Device can map host memory with
                                etrtHostAlloc/etrtHostGetDevicePointer */
  int computeMode;           /**< Compute mode (See ::etrtComputeMode) */
  int maxTexture1D;          /**< Maximum 1D texture size */
  int maxTexture1DMipmap;    /**< Maximum 1D mipmapped texture size */
  int maxTexture1DLinear;    /**< Maximum size for 1D textures bound to linear
                                memory */
  int maxTexture2D[2];       /**< Maximum 2D texture dimensions */
  int maxTexture2DMipmap[2]; /**< Maximum 2D mipmapped texture dimensions */
  int maxTexture2DLinear[3]; /**< Maximum dimensions (width, height, pitch) for
                                2D textures bound to pitched memory */
  int maxTexture2DGather[2]; /**< Maximum 2D texture dimensions if texture
                                gather operations have to be performed */
  int maxTexture3D[3];       /**< Maximum 3D texture dimensions */
  int maxTexture3DAlt[3];    /**< Maximum alternate 3D texture dimensions */
  int maxTextureCubemap;     /**< Maximum Cubemap texture dimensions */
  int maxTexture1DLayered[2];      /**< Maximum 1D layered texture dimensions */
  int maxTexture2DLayered[3];      /**< Maximum 2D layered texture dimensions */
  int maxTextureCubemapLayered[2]; /**< Maximum Cubemap layered texture
                                      dimensions */
  int maxSurface1D;                /**< Maximum 1D surface size */
  int maxSurface2D[2];             /**< Maximum 2D surface dimensions */
  int maxSurface3D[3];             /**< Maximum 3D surface dimensions */
  int maxSurface1DLayered[2];      /**< Maximum 1D layered surface dimensions */
  int maxSurface2DLayered[3];      /**< Maximum 2D layered surface dimensions */
  int maxSurfaceCubemap;           /**< Maximum Cubemap surface dimensions */
  int maxSurfaceCubemapLayered[2]; /**< Maximum Cubemap layered surface
                                      dimensions */
  size_t surfaceAlignment;         /**< Alignment requirements for surfaces */
  int concurrentKernels; /**< Device can possibly execute multiple kernels
                            concurrently */
  int ECCEnabled;        /**< Device has ECC support enabled */
  int pciBusID;          /**< PCI bus ID of the device */
  int pciDeviceID;       /**< PCI device ID of the device */
  int pciDomainID;       /**< PCI domain ID of the device */
  int tccDriver;         /**< 1 if device is a Tesla device using TCC driver, 0
                            otherwise */
  int asyncEngineCount;  /**< Number of asynchronous engines */
  int unifiedAddressing; /**< Device shares a unified address space with the
                            host */
  int memoryClockRate;   /**< Peak memory clock frequency in kilohertz */
  int memoryBusWidth;    /**< Global memory bus width in bits */
  int l2CacheSize;       /**< Size of L2 cache in bytes */
  int maxThreadsPerMultiProcessor; /**< Maximum resident threads per
                                      multiprocessor */
  int streamPrioritiesSupported;   /**< Device supports stream priorities */
  int globalL1CacheSupported;      /**< Device supports caching globals in L1 */
  int localL1CacheSupported;       /**< Device supports caching locals in L1 */
  size_t sharedMemPerMultiprocessor; /**< Shared memory available per
                                        multiprocessor in bytes */
  int regsPerMultiprocessor; /**< 32-bit registers available per multiprocessor
                              */
  int managedMemory;      /**< Device supports allocating managed memory on this
                             system */
  int isMultiDeviceBoard; /**< Device is on a multi-device board */
  int multiDeviceBoardGroupID; /**< Unique identifier for a group of devices on
                                  the same multi-device board */
  int hostNativeAtomicSupported; /**< Link between the device and the host
                                    supports native atomic operations */
  int singleToDoublePrecisionPerfRatio; /**< Ratio of single precision
                                           performance (in floating-point
                                           operations per second) to double
                                           precision performance */
  int pageableMemoryAccess; /**< Device supports coherently accessing pageable
                               memory without calling etrtHostRegister on it */
  int concurrentManagedAccess; /**< Device can coherently access managed memory
                                  concurrently with the CPU */
  int computePreemptionSupported; /**< Device supports Compute Preemption */
  int canUseHostPointerForRegisteredMem; /**< Device can access host registered
                                            memory at the same virtual address
                                            as the CPU */
  int cooperativeLaunch; /**< Device supports launching cooperative kernels via
                            ::etrtLaunchCooperativeKernel */
  int cooperativeMultiDeviceLaunch; /**< Device can participate in cooperative
                                       kernels launched via
                                       ::etrtLaunchCooperativeKernelMultiDevice
                                     */
  size_t sharedMemPerBlockOptin; /**< Per device maximum shared memory per block
                                    usable by special opt in */
};

/*
 * @brief Initalization values for an empty Device properties struct
 */
static const struct etrtDeviceProp etrtDevicePropDontCare = {
    {'\0'},    /* char   name[256];               */
    0,         /* size_t totalGlobalMem;          */
    0,         /* size_t sharedMemPerBlock;       */
    0,         /* int    regsPerBlock;            */
    0,         /* int    warpSize;                */
    0,         /* size_t memPitch;                */
    0,         /* int    maxThreadsPerBlock;      */
    {0, 0, 0}, /* int    maxThreadsDim[3];        */
    {0, 0, 0}, /* int    maxGridSize[3];          */
    0,         /* int    clockRate;               */
    0,         /* size_t totalConstMem;           */
    -1,        /* int    major;                   */
    -1,        /* int    minor;                   */
    0,         /* size_t textureAlignment;        */
    0,         /* size_t texturePitchAlignment    */
    //-1,        /* int    deviceOverlap;           */
    0,      /* int    multiProcessorCount;     */
    0,      /* int    kernelExecTimeoutEnabled */
    0,      /* int    integrated               */
    0,      /* int    canMapHostMemory         */
    0,      /* int    computeMode              */
    0,      /* int    maxTexture1D             */
    0,      /* int    maxTexture1DMipmap       */
    0,      /* int    maxTexture1DLinear       */
    {0, 0}, /* int    maxTexture2D[2]          */
    {
        0,
        0,
    },         /* int    maxTexture2DMipmap[2]    */
    {0, 0, 0}, /* int    maxTexture2DLinear[3]    */
    {0, 0},    /* int    maxTexture2DGather[2]    */
    {0, 0, 0}, /* int    maxTexture3D[3]          */
    {0, 0, 0}, /* int    maxTexture3DAlt[3]       */
    0,         /* int    maxTextureCubemap        */
    {0, 0},    /* int    maxTexture1DLayered[2]   */
    {0, 0, 0}, /* int    maxTexture2DLayered[3]   */
    {0, 0},    /* int    maxTextureCubemapLayered[2] */
    0,         /* int    maxSurface1D             */
    {0, 0},    /* int    maxSurface2D[2]          */
    {0, 0, 0}, /* int    maxSurface3D[3]          */
    {0, 0},    /* int    maxSurface1DLayered[2]   */
    {0, 0, 0}, /* int    maxSurface2DLayered[3]   */
    0,         /* int    maxSurfaceCubemap        */
    {0, 0},    /* int    maxSurfaceCubemapLayered[2] */
    0,         /* size_t surfaceAlignment         */
    0,         /* int    concurrentKernels        */
    0,         /* int    ECCEnabled               */
    0,         /* int    pciBusID                 */
    0,         /* int    pciDeviceID              */
    0,         /* int    pciDomainID              */
    0,         /* int    tccDriver                */
    0,         /* int    asyncEngineCount         */
    0,         /* int    unifiedAddressing        */
    0,         /* int    memoryClockRate          */
    0,         /* int    memoryBusWidth           */
    0,         /* int    l2CacheSize              */
    0,         /* int    maxThreadsPerMultiProcessor */
    0,         /* int    streamPrioritiesSupported */
    0,         /* int    globalL1CacheSupported   */
    0,         /* int    localL1CacheSupported    */
    0,         /* size_t sharedMemPerMultiprocessor; */
    0,         /* int    regsPerMultiprocessor;   */
    0,         /* int    managedMemory            */
    0,         /* int    isMultiGpuBoard          */
    0,         /* int    multiGpuBoardGroupID     */
    0,         /* int    hostNativeAtomicSupported */
    0,         /* int    singleToDoublePrecisionPerfRatio */
    0,         /* int    pageableMemoryAccess     */
    0,         /* int    concurrentManagedAccess  */
    0,         /* int    computePreemptionSupported */
    0,         /* int    canUseHostPointerForRegisteredMem */
    0,         /* int    cooperativeLaunch */
    0,         /* int    cooperativeMultiDeviceLaunch */
    0,         /* size_t sharedMemPerBlockOptin */
};             /**< Empty device properties */

typedef enum etrtError etrtError_t;
typedef struct ETstream_st *etrtStream_t;
typedef struct ETevent_st *etrtEvent_t;

/**
 * A three-integer-element data type
 */
struct uint3 {
  unsigned int x, y, z;
};

/**
 * A four-integer-element data type
 */
struct /*__builtin_align__(16)*/ uint4 {
  unsigned int x, y, z, w;
};

/**
 * A three-element array data type
 */
struct dim3 {
  unsigned int x, y, z;
#if defined(__cplusplus)
  dim3(unsigned int vx = 1, unsigned int vy = 1, unsigned int vz = 1)
      : x(vx), y(vy), z(vz) {}
  dim3(uint3 v) : x(v.x), y(v.y), z(v.z) {}
  operator uint3(void) {
    uint3 t;
    t.x = x;
    t.y = y;
    t.z = z;
    return t;
  }
#endif /* __cplusplus */
};

typedef struct uint3 uint3;
typedef struct uint4 uint4;
typedef struct dim3 dim3;

/*
 * @brief ET Runtime Supported Datatypes
 */
typedef enum etrtDataType_t {
  ETRT_R_16F = 2,  /* real as a half */
  ETRT_C_16F = 6,  /* complex as a pair of half numbers */
  ETRT_R_32F = 0,  /* real as a float */
  ETRT_C_32F = 4,  /* complex as a pair of float numbers */
  ETRT_R_64F = 1,  /* real as a double */
  ETRT_C_64F = 5,  /* complex as a pair of double numbers */
  ETRT_R_8I = 3,   /* real as a signed char */
  ETRT_C_8I = 7,   /* complex as a pair of signed char numbers */
  ETRT_R_8U = 8,   /* real as a unsigned char */
  ETRT_C_8U = 9,   /* complex as a pair of unsigned char numbers */
  ETRT_R_32I = 10, /* real as a signed int */
  ETRT_C_32I = 11, /* complex as a pair of signed int numbers */
  ETRT_R_32U = 12, /* real as a unsigned int */
  ETRT_C_32U = 13  /* complex as a pair of unsigned int numbers */
} etrtDataType;

/*
 * @brief ET Runtime Library Properties
 *
 * Fields that make up the library's properties struct
 */
typedef enum libraryPropertyType_t {
  MAJOR_VERSION,
  MINOR_VERSION,
  PATCH_LEVEL
} libraryPropertyType;

#endif // ETRT_BIN_H
