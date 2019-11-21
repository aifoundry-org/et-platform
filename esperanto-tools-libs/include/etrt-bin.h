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

#include "Common/ErrorTypes.h"
#include "Common/ProjectAutogen.h"
#include "Core/Stream.h"

#include <stddef.h>


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


typedef enum etrtError etrtError_t;
typedef class et_runtime::Stream *etrtStream_t;
typedef class et_runtime::Stream EtStream;
typedef class et_runtime::Event* etrtEvent_t;


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

#endif // ETRT_BIN_H
