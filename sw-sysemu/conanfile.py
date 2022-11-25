from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import get, rmdir
import os

required_conan_version = ">=1.52.0"


class SwSysemuConan(ConanFile):
    name = "sw-sysemu"
    url = "https://gitlab.esperanto.ai/software/sw-sysemu"
    description = "The functional ETSOC-1 emulator. Able to execute RISC-V instructions with Esperanto extensions"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "profiling": [True, False],
        "backtrace": [True, False],
        "preload_elfs": [True, False],
        "preload_compression": [None, "lz4"],
    }
    default_options = {
        "shared": False,
        "profiling": False,
        "backtrace": False,
        "preload_elfs": False,
        "preload_compression": "lz4",
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/sw-sysemu.git",
        "revision": "auto",
    }

    python_requires = "conan-common/[>=0.5.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, "sw-sysemu")

    def requirements(self):
        self.requires("glog/0.4.0")
        self.requires("elfio/3.8")
        self.requires("lz4/1.9.3")
        if self.options.backtrace:
            self.requires("libunwind/1.5.0")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

        if self.settings.os != "Linux":
            self.output.warn("%s has only been tested under Linux. You're on your own" % self.name)

    def layout(self):
        cmake_layout(self)
        self.cpp.source.includedirs = [".", "sw-sysemu/include"]

        self.cpp.package.libs = ["fpu", "sysemu", "sw-sysemu"]
        self.cpp.package.includedirs = ["include"]

        libfpu_comp_name = "libfpu"
        libfpu_cmake_name = "fpu"
        self.cpp.package.components[libfpu_comp_name].set_property("cmake_target_name", libfpu_cmake_name)
        self.cpp.package.components[libfpu_comp_name].requires = []
        self.cpp.package.components[libfpu_comp_name].libs = ["fpu"]
        self.cpp.package.components[libfpu_comp_name].includedirs =  ["include"]
        self.cpp.build.components[libfpu_comp_name].libs = ["fpu"]
        self.cpp.build.components[libfpu_comp_name].libdirs = ["."]
        self.cpp.source.components[libfpu_comp_name].includedirs = [".", "fpu"]


        libsysemu_comp_name = "libsysemu"
        libsysemu_cmake_name = "sysemu"
        self.cpp.package.components[libsysemu_comp_name].set_property("cmake_target_name", libsysemu_cmake_name)
        self.cpp.package.components[libsysemu_comp_name].requires = ["elfio::elfio", libfpu_comp_name]
        self.cpp.package.components[libsysemu_comp_name].libs = ["sysemu"]
        self.cpp.package.components[libsysemu_comp_name].defines = ["SYS_EMU"]
        self.cpp.package.components[libsysemu_comp_name].includedirs =  ["include"]
        self.cpp.package.components[libsysemu_comp_name].libdirs = ["lib"]
        if self.options.backtrace:
            self.cpp.package.components[libsysemu_comp_name].requires.append("libunwind::libunwind")
        self.cpp.build.components[libsysemu_comp_name].requires = ["elfio::elfio", libfpu_comp_name]
        self.cpp.build.components[libsysemu_comp_name].libs = ["sysemu"]
        self.cpp.build.components[libsysemu_comp_name].defines = ["SYS_EMU"]
        self.cpp.build.components[libsysemu_comp_name].libdirs = ["."]
        self.cpp.source.components[libsysemu_comp_name].includedirs = ["."]

        libsw_sysemu_comp_name = "libsw-sysemu"
        libsw_sysemu_cmake_name = "sw-sysemu"
        self.cpp.package.components[libsw_sysemu_comp_name].set_property("cmake_target_name", libsw_sysemu_cmake_name)
        self.cpp.package.components[libsw_sysemu_comp_name].requires = [libsysemu_comp_name, libfpu_comp_name, "glog::glog"]
        self.cpp.package.components[libsw_sysemu_comp_name].libs = ["sw-sysemu"]
        self.cpp.build.components[libsw_sysemu_comp_name].requires = [libsysemu_comp_name, libfpu_comp_name, "glog::glog"]
        self.cpp.build.components[libsw_sysemu_comp_name].libs = ["sw-sysemu"]
        self.cpp.build.components[libsw_sysemu_comp_name].libdirs = ["."]
        self.cpp.source.components[libsw_sysemu_comp_name].includedirs = ["sw-sysemu/include", "sw-sysemu/include/sw-sysemu"]

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["PROFILING"] = self.options.profiling
        tc.variables["BACKTRACE"] = self.options.backtrace
        tc.variables["PRELOAD_LZ4"] = "lz4" is self.options.preload_compression
        if self.options.preload_elfs:
            emmedded_elfs_conanfile = os.path.join(self.source_folder, "conanfile_embedded_elfs.txt")
            self.run(f"conan install {emmedded_elfs_conanfile} -pr:b default -pr:h baremetal-rv64-gcc8.2-release --build missing -g deploy -if={self.build_folder}")
            preload_elfs_list = [
                os.path.join(self.build_folder, "device-bootloaders/lib/esperanto-fw/BootromTrampolineToBL2/BootromTrampolineToBL2.elf"),
                os.path.join(self.build_folder, "device-bootloaders/lib/esperanto-fw/ServiceProcessorBL2/fast-boot/ServiceProcessorBL2_fast-boot.elf"),
                os.path.join(self.build_folder, "device-minion-runtime/lib/esperanto-fw/MachineMinion/MachineMinion.elf"),
                os.path.join(self.build_folder, "device-minion-runtime/lib/esperanto-fw/MasterMinion/MasterMinion.elf"),
                os.path.join(self.build_folder, "device-minion-runtime/lib/esperanto-fw/WorkerMinion/WorkerMinion.elf"),
            ]
            tc.variables["PRELOAD_ELFS"] = ";".join(preload_elfs_list)
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        # utilities
        bin_path = os.path.join(self.package_folder, "bin") if self.package_folder else "bin"
        bin_ext = ".exe" if self.settings.os == "Windows" else ""

        self.output.info("Appending PATH env var with : {}".format(bin_path))
        self.env_info.PATH.append(bin_path)

        sys_emu = os.path.join(bin_path, "sys_emu" + bin_ext)
        self.output.info("Setting SYS_EMU to {}".format(sys_emu))
        self.env_info.SYS_EMU = sys_emu
