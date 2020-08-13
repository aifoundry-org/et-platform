.. _KernelLaunch:

***********************************
Launching a Kernel With the Runtime
***********************************

This is a quick guide describing the necesssary steps and class interactions
that will enable you to interact wit the Esperanto accelerate and launch a
kernel on the device.

The following guide assumes that you have already loaded the Esperanto PCIe driver
in your system and the necessary character devices have been created.

The necessary steps are described in the following Sections


Instantiate the DeviceManager
=============================

.. code-block:: c++
    :linenos:
    :lineno-start: 1

    auto device_manager = et_runtime::getDeviceManager();



Create A Device
===============

.. code-block:: c++
    :linenos:
    :lineno-start: 1

    auto ret_value = device_manager->registerDevice(0);
    dev_ = ret_value.get();

    auto worker_minion = absl::GetFlag(FLAGS_worker_minion_elf);
    auto machine_minion = absl::GetFlag(FLAGS_machine_minion_elf);
    auto master_minion = absl::GetFlag(FLAGS_master_minion_elf);

    // Start the simulator and load device-fw in memory
    dev_->setFWFilePaths({master_minion, machine_minion, worker_minion});

    ASSERT_EQ(dev_->init(), etrtSuccess);


Create A Kernel
===============

.. code-block:: c++
    :linenos:
    :lineno-start: 1

    auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
    fs::path uberkernel = fs::path(kernels_dir) / fs::path("uberkernel.elf");
    auto &registry = dev_->codeRegistry();

    // Register a 2 layer uberkernel
    auto register_res =
        registry.registerUberKernel("main",
                                    {{Kernel::ArgType::T_layer_dynamic_info},
                                     {Kernel::ArgType::T_layer_dynamic_info}},
                                    uberkernel.string());
    ASSERT_TRUE((bool)register_res);
    auto &kernel = std::get<1>(register_res.get());

    auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
    ASSERT_EQ(load_res.getError(), etrtSuccess);
    auto module_id = load_res.get();



Construct The Kernel Arguments
==============================


.. code-block:: c++
    :linenos:
    :lineno-start: 1

    int array_size = 200;
    int size = sizeof(uint64_t) * array_size;
    BufferDebugInfo info;
    auto malloc_res= dev_->mem_manager().mallocConstant(size, info);
    ASSERT_TRUE((bool)malloc_res);
    auto device_buffer = malloc_res.get();

    Kernel::layer_dynamic_info_t layer_info = {};
    layer_info.tensor_a = reinterpret_cast<uint64_t>(device_buffer.ptr());
    layer_info.tensor_b = size;
    Kernel::LaunchArg arg;
    arg.type = Kernel::ArgType::T_layer_dynamic_info;
    arg.value.layer_dynamic_info = layer_info;

    int array_size_2 = 400;
    size = sizeof(uint64_t) * array_size_2;
    malloc_res = dev_->mem_manager().mallocConstant(size, info);
    ASSERT_TRUE((bool)malloc_res);
    auto device_buffer_2 = malloc_res.get();

    layer_info = {};
    layer_info.tensor_a = reinterpret_cast<uint64_t>(device_buffer_2.ptr());
    layer_info.tensor_b = size;
    Kernel::LaunchArg arg2;
    arg2.type = Kernel::ArgType::T_layer_dynamic_info;
    arg2.value.layer_dynamic_info = layer_info;


Launch A Kernel Launch
======================

.. code-block:: c++
    :linenos:
    :lineno-start: 1

    auto args = std::vector<std::vector<Kernel::LaunchArg>>({{arg}, {arg2}});
    auto launch = kernel.createKernelLaunch(args);
    auto launch_res = launch->launchBlocking(&dev_->defaultStream());
    ASSERT_EQ(launch_res, etrtSuccess);


Copy Memory Back
================

.. code-block:: c++
    :linenos:
    :lineno-start: 1

    ///  Check writing from the first layer
    {
      std::vector<uint64_t> data(array_size, 0xEEEEEEEEEEEEEEEEULL);
      std::vector<uint64_t> refdata(array_size, 0xBEEFBEEFBEEFBEEFULL);
      auto res = dev_->memcpy(HostBuffer(data.data()), device_buffer, array_size * sizeof(uint64_t));
      ASSERT_EQ(res, etrtSuccess);
      ASSERT_THAT(data, ::testing::ElementsAreArray(refdata));
    }

    ///  Check writing from the second layer
    {
      std::vector<uint64_t> data(array_size_2, 0xEEEEEEEEEEEEEEEEULL);
      std::vector<uint64_t> refdata(array_size_2, 0xBEEFBEEFBEEFBEEFULL);
      auto res =
        dev_->memcpy(HostBuffer(data.data()), device_buffer_2, array_size_2 * sizeof(uint64_t));
      ASSERT_EQ(res, etrtSuccess);
      ASSERT_THAT(data, ::testing::ElementsAreArray(refdata));
    }
