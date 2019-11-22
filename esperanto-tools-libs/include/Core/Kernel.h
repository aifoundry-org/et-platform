//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_KERNEL_H
#define ET_RUNTIME_KERNEL_H

#include "etrt-bin.h"

#include <string>
#include <vector>

namespace et_runtime {

class Stream;

///
/// The whole purpose of the ET SoC is to run many instances of Kernels in
/// parallel to accelerate computationally complex applications.
///
/// The following functions are used to define a Kernel to be executed, the
/// parameters that are to be given to each instance of the Kernel, and a
/// specification of how many Kernel instances to launch.
///
/// **Kernels**
///
/// A Kernel can be specified in many ways (e.g., through a variety of DSLs,
/// high-level languages, or assembly code), but must ultimately be turned in a
/// block of RV64I??? (with ET ISA Extensions) machine code (by an AOT or JIT
/// compiler or an assembler).  These blocks of code (i.e., Kernels) are managed
/// (both in files and in memory) in standard ELF format, stripped of symbols,
/// and with a two entry points -- one for each of the two HARTs in a Minion. In
/// this way, each Kernel instance can have at most two threads executing on a
/// Minion at a time.  It is anticipated that the Primary thread will perform
/// the bulk of the computation, while the Secondary thread coordinates with the
/// Primary thread and performs data pre-fetch operations. However, there are
/// restrictions that limit this to be the only use case supported.
///
/// N.B. There are constraints on the Minion implementation that make the two
/// threads not totally identical in capability -- e.g., there is only a
/// Register File for the Primary thread in the Vector Processing Unit.
///
/// @todo  Define and implement a mechanism for specifying Kernel thread-pairs
/// -- i.e., need to be able to define the additional code for the second Minion
/// thread/HART.  Assume that both Primary and Secondary threads get the same
/// set of pre-defined stack variables.
///
/// **Kernel Differentiation**
///
/// The whole point of the Device is to run concurrently, multiple instances of
/// a Kernel to accelerate a particular computation on some data.  To this end,
/// each Kernel instance is given access to the memory regions where the input
/// data can be read and where the output data can be written.  Given that the
/// code that implements each of the Kernel instances is identical, some means
/// of differentiation is required to ensure that the work/data is properly
/// distributed across all of the Kernels in a given computational step. The
/// performance of the computational step to be accelerated is a function of how
/// efficiently the total work can be distributed among Kernels, and their
/// (partial) results combined to create the complete result. In a ideal world,
/// a parallelizing compiler would determine how to decompose a given
/// computation into discrete units and it would be able to generate Kernels
/// that would make the appropriate memory references to prefetch, load, and
/// operate upon their individual portion of the problem.  In the absence of
/// such a magic compiler, we rely on the programmer to define both the number
/// and the organization of Kernel instances to be executed.
///
/// For simplicity of exposition here, from this point on, we assume that each
/// Kernel instance will be executing a single thread of control (despite the
/// fact that there will frequently be thread pairs executing concurrently on
/// each Minion).
///
/// This API has chosen to follow the method of differentiation used by CUDA --
/// namely, a two-level, multidimensional, indexing scheme for defining the
/// number and organization of Kernel instances. This scheme defines the number
/// of Kernel instances and how they are differentiated. The lower level of
/// indexing defines the number of threads to be launched in an affinity group
/// known as a Block, while the higher level of indexing defines the number of
/// blocks that should be executed together in a Grid.
///
/// The total number of Kernel instances for a given Kernel launch will be the
/// number of threads per Block times the number of Blocks.
///
/// Threads within a Block are can communicate among themselves, while threads
/// in different blocks do not communicate -- therefore threads in a Block are
/// considered to be more tightly coupled, and Blocks in a Blocks within Grids
/// are considered to be largely independent of each other.
///
/// Grid and Block values are specified for each Kernel launch call to this API,
/// and the Device execution environment uses these values to ensure that the
/// desired number of Kernel instances are dispatched to the appropriate
/// Minions. The Device execution environment also ensures that each Kernel
/// instance is given its own, unique, set of index values as input parameters
/// in order to differentiate itself. Each Kernel instance is given its unique
/// values in the following stack variables: `gridDim`, `blockDim`, `blockIdx`,
/// and `threadIdx`.  Each of these variables is a three-component vector that
/// can represent a 1D, 2D, or 3D value.  The total number of items defined by
/// each of these variables is the product of all of their dimensions -- i.e.,
/// if a Kernel is launched with the values `gridDim`=(4,4,4) and
/// `blockDim`=(8,1,1), then a total of 512 Kernel instances are to be
/// instantiated (organized as a four-by-four cube of eight-element vectors of
/// Kernels), and the first Kernel instance will see: `blockIdx`=(0,0,0) and
/// `threadIdx`=(0,0,0), the next instance will see: `blockIdx`=(0,0,0) and
/// `threadIdx`=(0,0,1), and so on until the last instance that will see:
/// `blockIdx`=(3,3,3) and `threadIdx`=(7,0,0).
///
/// In addition to these pre-defined values, the Kernel code has access to the
/// ID of the Minion, its Neighborhood, and Shire, which can also used to
/// further differentiate instances.
///
/// The choice of indexing values is a function of the size of the computation
/// to be performed, the number of processing elements available, and their
/// structure (in terms of both memory and intercommunication). Ideally, you'd
/// like to write code that was independent of the details of the HW
/// accelerator, but that's not really practical, so there are guidelines on how
/// to structure/decompose/distribute problems to work best on a given piece of
/// HW. At a high level, Blocks map well onto Neighborhoods and Grids onto
/// Shires, but unfortunately, it's not that simple and more effort has to be
/// put into decomposing problems to achieve peak performance on the ET SoC.
///
/// @see(ET SoC Programmer's Guide).
///
/// **API Kernel Launch Calls**
///
/// Clients of this API define the number of (and indexing scheme for) instances
/// of each Kernel that is to be executed, the arguments to be passed all Kernel
/// instances, and the (potentially dual-threaded) Kernel to be executed. This
/// API passes the necessary information to the currently attached Device and
/// the Device handles the dispatching of Kernel instances.
///
/// **Kernel Distribution and Execution***
///
/// The ::etrtLaunch() call causes the Device runtime environment to set up
/// on-chip memory and dispatch all of the desired Kernel instances.  Once
/// launched, Kernel instances run to completion, and signal the Device
/// execution environment, which in trun is responsible for managing the
/// completion of the execution of the computational step defined by the Kernel
/// in the API call.
///
/// The Device runtime keeps track of the state of the Minions and knows how
/// many are occupied with Kernel instances, and how many can have instances
/// dispatched upon them. If a Kernel launch call requires fewer threads than
/// there are currently available Minions, the entire set of Kernel instances
/// are dispatched and run concurrently.  In this case, it's expected that all
/// instances should complete their execution in close to the same amount of
/// time. On the other hand, if a launch calls for more instances than there are
/// available Minions, then there are several optional behaviors that can be
/// selected on a per-launch-call basis -- an `etrtErrorLaunchOutOfReources`
/// error could be returned; the call could block until until the required
/// number of Minions are available (reserving all available Minions until
/// sufficient ones are acquired), or as many instances can be dispatched as
/// there are available Minions and the execution of the individual Kernel
/// instances are serialized until all of the required work has been completed.
///
/// @todo  Document the constraints that can be applied on kernel launches --
/// e.g., Block/Neighborhood or Grid/Shire affinity/anti-fragmentation options,
/// kernel gravity, allocation quantization, etc.
///
////
class Kernel {
public:
  Kernel(const std::string &name);

  ///
  /// @brief  Set up the indexing configuration for Kernel launches.
  ///
  /// Define the Grid and Block dimensions for subsequent Kernel launches, as
  /// well as the maximum amount of shared memory that the Kernel instances can
  /// consume.
  ///
  /// @todo  Document the Kernel launch options selected by the flags argument.
  ///
  /// @param[in] gridDim  The higher-level (i.e., Grid) index dimensions for
  /// Kernels to be launched.
  /// @param[in] blockDim  The lower-level (i.e., Block) indexes dimensions
  /// Kernels to be launched.
  /// @param[in] sharedMem  Maximum number of bytes of shared memory that can be
  /// used by the launched Kernel instances.
  /// @return  etrtSuccess, etrtErrorInvalidConfiguration,
  /// etrtErrorLaunchOutOfResources
  ///
  /// FIXME SW-1362
  // ///etrtError etrtConfigureCall(dim3 gridDim, dim3 blockDim, size_t
  // sharedMem);

  ///
  /// @brief Append an argument to the Kenel launch
  ///
  /// Append to the list of launch arguments the value
  ///
  /// @param[in] value  Argument value to be passed to the kernel
  /// @return  etrtSuccess, \todo Add exit codes
  template <typename T> etrtError addArgument(const T &value);

  ///
  /// @brief Launch the kernel on the device.
  etrtError launch();
};
} // namespace et_runtime
#endif // ET_RUNTIME_KERNEL_H
