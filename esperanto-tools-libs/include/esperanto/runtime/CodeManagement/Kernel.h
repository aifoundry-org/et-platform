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

/// @file

#include "esperanto/runtime/Common/CommonTypes.h"
#include "esperanto/runtime/Support/ErrorOr.h"

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace et_runtime {

class Device;
class Stream;

namespace device_api {
namespace devfw_commands {

class KernelLaunchCmd;
}
} // namespace device_api

/// @class Kernel Kernel.h
/// FIXME the following comment is outdate and does not correspond to the
/// current implementation
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
  /// @struct layer_dynamic_info
  ///
  /// Struct holding the current version of the activation record
  struct layer_dynamic_info_t {
    uint64_t tensor_a;  // Pointer to tensor A
    uint64_t tensor_b;  // Pointer to tensor B
    uint64_t tensor_c;  // Pointer to tensor C
    uint64_t tensor_d;  // Pointer to tensor D
    uint64_t tensor_e;  // Pointer to tensor E
    uint64_t tensor_f;  // Pointer to tensor F
    uint64_t tensor_g;  // Pointer to tensor G
    uint64_t tensor_h;  // Pointer to tensor H
    uint64_t kernel_id; // Id for this Kernel
  };

  enum class ArgType : uint8_t {
    None = 0,
    T_int8 = 1,
    T_uint8 = 2,
    T_int16 = 3,
    T_uint16 = 4,
    T_int32 = 5,
    T_uint32 = 6,
    T_float = 7,
    T_double = 8,
    T_tensor = 9,
    T_layer_dynamic_info = 10,
    Num
  };

  // FIXME we shoule move to avariant but that requires c++17
  /// @union Value of the individual argument values to pass to a kernel launch
  union ArgValue {
    int8_t int8;
    uint8_t uint8;
    int16_t int16;
    uint16_t uint16;
    int32_t int32;
    uint32_t uint32;
    float vfloat;
    double vdouble;
    // FIXME define tesnor type
    layer_dynamic_info_t layer_dynamic_info;
  };

  struct LaunchArg {
    ArgType type;
    ArgValue value;
  };

  /// @brief Return string with the name of the argument type
  static const std::string &argTypeToStr(ArgType arg);

  /// @brief Return the size in bytes of the argument type
  static ssize_t argTypeSize(ArgType arg);

  /// @Breif Return vector with the raw bytes of an argument
  static std::vector<uint8_t> argBytes(const LaunchArg &arg);

  /// @class KernelLaunch
  ///
  /// @brief Launch a kernel on the devie with the specified arguments
  //
  class KernelLaunch {
  public:
    ///
    /// @param[in] kernel Reference to a registered kernel
    /// @param[in] args  Vector with the arguments to pass to the kernel and
    /// their type
    ///   Their types will be checked against the registered signature of the
    ///   kernel
    KernelLaunch(const Kernel &kernel, std::vector<LaunchArg> &args);

    /// @brief Launch the kernel and wait for the result
    ///
    /// This is a blocking kernel launch the function will block until a result
    /// is received from the device
    ///
    /// @param[in] stream  Stream to launch this kernel on
    /// @returns etrtSuccess or error describing the kernel launch status
    etrtError launchBlocking(Stream *stream);

    /// @brief Non blocking kernel launch
    ///
    /// @param[in] stream  Stream to launch this kernel on
    /// @returns etrtSuccess or error describing the kernel launch status
    etrtError launchNonBlocking(Stream *stream);

  private:
    const Kernel &kernel_;
    std::vector<LaunchArg> args_;

    /// @brief Helper function for launching a kernel
    ErrorOr<std::shared_ptr<device_api::devfw_commands::KernelLaunchCmd>>
    launchHelper(Stream *stream);
  };

  /// @brief Construct a new kernel
  ///
  /// @param[in] name String with the unique name of the kernel across the whole
  /// system
  /// @param[in] arg_list Vector with type of argumenrs this kernel can accept
  /// @param[in] mid  ModuleID of the CodeModue/ELF that contains the kernel
  Kernel(const std::string &name, const std::vector<ArgType> &arg_list,
         CodeModuleID mid);

  virtual ~Kernel() = default;

  /// @return Return the unique ID of this registered kernel function
  KernelCodeID id() const { return id_; }

  /// @return Return the Module ID where this kernel is defined
  CodeModuleID moduleID() const { return mid_; }

  /// @return Name of the kernel
  const std::string &name() const { return name_; }

  /// @return List of argument types
  const std::vector<ArgType> &kernelArgs() const { return kernel_args_; }

  /// @brief Create a Kernel Launch object with the given list of argument
  /// values
  /// @return Pointer to the created kernel launch object
  std::unique_ptr<KernelLaunch>
  createKernelLaunch(std::vector<LaunchArg> &args);

  /// @brief Search for the kernel object by ID
  static ErrorOr<Kernel &> findKernel(KernelCodeID id);

  /// @brief Return the entry point address of the kernel on the device
  ///
  /// If the ELF is not loaded on the device then load the module on the device
  /// "lazily"
  ///
  /// @returns Error if we failed to load the module on the device or the PC of
  /// the kenrel
  ErrorOr<uintptr_t> kernelEntryPoint() const;

protected:
  /// Delegating constructor that is going to do the basic initialization for
  /// this class and any children classes. No logging is performed as part of
  /// this constructor
  ///
  /// @param mid ModuleID this kernel is part of
  Kernel(CodeModuleID mid);

  /// @brief Register the kernel in the global kernel registry
  ///
  /// @param[in] name Name fo the kernel
  /// @param[in] kernel Pointer ot the allocated kernel. This pointer will be
  /// stored in a std::unique_ptr so that all kernel objects get automatically
  /// destroyed when the static kernel registry gets destroyed
  ///
  /// @return True on success, False if the kernel has already been registered
  static bool registerKernel(const std::string &name, Kernel *kernel);

  KernelCodeID id_ = -1; ///< ID of the current kernel
  CodeModuleID mid_ =
      -1; ///< ID of the module (ELF) where this kernel is located
  std::string name_;
  std::vector<ArgType> kernel_args_;

  static KernelCodeID
      global_kernel_id_; ///< Static counter with the total number of kernels
                         ///< recorded by the runtime
  using KernelMap =
      std::unordered_map<std::string,
                         std::tuple<KernelCodeID, std::unique_ptr<Kernel>>>;
  static KernelMap kernel_registry_; ///< Static map that keeps track of all the
                                     ///< kernels registered in the runtime
};
} // namespace et_runtime

/// @brief Helper overload of operator<< that will allow us to print a vector of
/// kenrel laucnh arguments
std::ostream &operator<<(std::ostream &os,
                         const std::vector<et_runtime::Kernel::LaunchArg> &vec);

#endif // ET_RUNTIME_KERNEL_H
