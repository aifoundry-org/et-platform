from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
from conans.errors import ConanInvalidConfiguration
import textwrap
import os

required_conan_version = ">=1.36.0"

class DeviceMinionRuntimeConan(ConanFile):
    name = "device-minion-runtime"
    version = "0.0.1"
    url = "https://gitlab.esperanto.ai/software/device-minion-runtime"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {}
    default_options = {}

    generators = "cmake_find_package_multi"

    exports_sources = [ "CMakeLists.txt", "cmake/*", "external/*", "include/*", "scripts/*", "src/*", "tools/*", "EsperantoDeviceMinionRuntimeConfig.cmake.in", "EsperantoDeviceMinionRuntimeConfigVersion.cmake.in" ]

    requires = [
        "deviceApi/0.1.0",
        "signedImageFormat/1.0" 
    ]

    def build_requirements(self):
        self.build_requires("cmake-modules/[>=0.4.1 <1.0.0]", force_host_context=True)

    def generate(self):
        new_cmake_flags_init_template = textwrap.dedent("""
        set(CMAKE_CXX_FLAGS_INIT "${CONAN_CXX_FLAGS}" CACHE STRING "" FORCE)
        set(CMAKE_C_FLAGS_INIT "${CONAN_C_FLAGS} -std=gnu11 -fno-zero-initialized-in-bss -ffunction-sections -fdata-sections -fstack-usage -Wall -Wextra -Werror -Wdouble-promotion -Wformat -Wnull-dereference -Wswitch-enum -Wshadow -Wstack-protector -Wpointer-arith -Wundef -Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wno-main" CACHE STRING "" FORCE)
        set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CONAN_SHARED_LINKER_FLAGS}" CACHE STRING "" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_INIT "${CONAN_EXE_LINKER_FLAGS}" CACHE STRING "" FORCE)
        """)

        tc = CMakeToolchain(self)
        tc.variables["ENABLE_STRICT_BUILD_TYPES"] = True
        tc.variables["DEVICE_MINION_RUNTIME_DEPRECATED"] = True
        tc.variables["BUILD_DOC"] = False
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.deps_cpp_info["cmake-modules"].rootpath, "cmake")
        tc.preprocessor_definitions["RISCV_ET_MINION"] = ""
        tc.blocks["cmake_flags_init"].template = new_cmake_flags_init_template
        tc.generate()
    

    _cmake = None
    def _configure_cmake(self):
        if not self._cmake:
            cmake = CMake(self)
            cmake.verbose = True
            cmake.configure()
            self._cmake = cmake
        return self._cmake
    
    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
    
    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
    
    def package_info(self):
        etfw_includedir = os.path.join("include", "esperanto-fw")
        etfw_libdir = os.path.join("lib", "esperanto-fw")
        
        self.cpp_info.set_property("cmake_target_name", "EsperantoDeviceMinionRuntime")
        self.cpp_info.components["device_configuration"].requires = ["signedImageFormat::signedImageFormat"]

        self.cpp_info.components["device_common"].includedirs = ["include", etfw_includedir, os.path.join(etfw_includedir, "device-common")]

        self.cpp_info.components["device_trace"].includedirs = ["include", etfw_includedir, os.path.join(etfw_includedir, "device_trace")]
        self.cpp_info.components["device_trace"].libdirs = [etfw_libdir]
        self.cpp_info.components["device_trace"].libs = ["device_trace"]
        self.cpp_info.components["device_trace"].defines = ["-DSP_OR_COMPUTE_MINION=1"]
        self.cpp_info.components["device_trace"].requires = ["device_common"]

        self.cpp_info.components["device_trace_mm"].includedirs = ["include", etfw_includedir, os.path.join(etfw_includedir, "device_trace")]
        self.cpp_info.components["device_trace_mm"].libdirs = [etfw_libdir]
        self.cpp_info.components["device_trace_mm"].libs = ["device_trace_mm"]
        self.cpp_info.components["device_trace_mm"].defines = ["-DMASTER_MINION=1"]
        self.cpp_info.components["device_trace_mm"].requires = ["device_common"]

        self.cpp_info.components["sp_firmware_helpers"].includedirs = ["include", etfw_includedir, os.path.join(etfw_includedir, "firmware_helpers")]
        self.cpp_info.components["sp_firmware_helpers"].libs = ["sp_firmware_helpers"]
        self.cpp_info.components["sp_firmware_helpers"].libdirs = [etfw_libdir]
        self.cpp_info.components["sp_firmware_helpers"].requires = ["device_common"]

        self.cpp_info.components["mm_firmware_helpers"].includedirs = ["include", etfw_includedir, os.path.join(etfw_includedir, "firmware_helpers")]
        self.cpp_info.components["mm_firmware_helpers"].libs = ["mm_firmware_helpers"]
        self.cpp_info.components["mm_firmware_helpers"].libdirs = [etfw_libdir]
        self.cpp_info.components["mm_firmware_helpers"].requires = ["device_common"]

        self.cpp_info.components["MachineMinion"].bindirs = [os.path.join(etfw_libdir, "MachineMinion")]
        self.cpp_info.components["MachineMinion"].requires = ["device_configuration"]
        self.cpp_info.components["MasterMinion"].bindirs = [os.path.join(etfw_libdir, "MasterMinion")]
        self.cpp_info.components["MasterMinion"].requires = ["deviceApi::deviceApi", "device_trace_mm", "device_common", "device_configuration"]
        self.cpp_info.components["WorkerMinion"].bindirs = [os.path.join(etfw_libdir, "WorkerMinion")]
        self.cpp_info.components["WorkerMinion"].requires = ["deviceApi::deviceApi", "device_configuration"]
