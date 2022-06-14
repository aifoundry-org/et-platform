from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake
from conan.tools.layout import cmake_layout
from conans import tools
import os
import re


class SwSysemuConan(ConanFile):
    name = "sw-sysemu"
    url = "https://gitlab.esperanto.ai/software/sw-sysemu"
    description = "The functional ETSOC-1 emulator. Able to execute RISC-V instructions with Esperanto extensions"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "profiling": [True, False],
        "backtrace": [True, False]
    }
    default_options = {
        "shared": False,
        "profiling": False,
        "backtrace": False
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/sw-sysemu.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"

    python_requires = "conan-common/[>=0.5.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, "sw-sysemu")

    def requirements(self):
        self.requires("glog/0.4.0")
        self.requires("elfio/3.8")
        if self.options.backtrace:
            self.requires("libunwind/1.5.0")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

        if self.settings.os != "Linux":
            self.output.warn("%s has only been tested under Linux. You're on your own" % self.name)

    def layout(self):
        cmake_layout(self)
        self.cpp.source.includedirs = ["."]

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["PROFILING"] = self.options.profiling
        tc.variables["BACKTRACE"] = self.options.backtrace
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        # library components
        libsysemu_comp_name = "libsysemu"
        libsysemu_cmake_name = "sysemu"
        self.cpp_info.components[libsysemu_comp_name].names["cmake_find_package"] = libsysemu_cmake_name # deprecated (remove with conan 2.0)
        self.cpp_info.components[libsysemu_comp_name].names["cmake_find_package_multi"] = libsysemu_cmake_name # deprecated (remove with conan 2.0)
        self.cpp_info.components[libsysemu_comp_name].set_property("cmake_target_name", libsysemu_cmake_name)
        self.cpp_info.components[libsysemu_comp_name].requires = ["elfio::elfio"]
        self.cpp_info.components[libsysemu_comp_name].libs = ["sysemu"]
        self.cpp_info.components[libsysemu_comp_name].defines = ["SYS_EMU"]
        self.cpp_info.components[libsysemu_comp_name].includedirs =  ["include"]
        self.cpp_info.components[libsysemu_comp_name].libdirs = ["lib"]
        if self.options.backtrace:
            self.cpp_info.components[libsysemu_comp_name].requires.append("libunwind::libunwind")

        libfpu_comp_name = "libfpu"
        libfpu_cmake_name = "fpu"
        self.cpp_info.components[libfpu_comp_name].names["cmake_find_package"] = libfpu_cmake_name # deprecated (remove with conan 2.0)
        self.cpp_info.components[libfpu_comp_name].names["cmake_find_package_multi"] = libfpu_cmake_name # deprecated (remove with conan 2.0)
        self.cpp_info.components[libfpu_comp_name].set_property("cmake_target_name", libfpu_cmake_name)
        self.cpp_info.components[libfpu_comp_name].requires = []
        self.cpp_info.components[libfpu_comp_name].libs = ["fpu"]
        self.cpp_info.components[libfpu_comp_name].includedirs =  ["include"]
        self.cpp_info.components[libfpu_comp_name].libdirs = ["lib"]

        libsw_sysemu_comp_name = "libsw-sysemu"
        libsw_sysemu_cmake_name = "sw-sysemu"
        self.cpp_info.components[libsw_sysemu_comp_name].names["cmake_find_package"] = libsw_sysemu_cmake_name # deprecated (remove with conan 2.0)
        self.cpp_info.components[libsw_sysemu_comp_name].names["cmake_find_package_multi"] = libsw_sysemu_cmake_name # deprecated (remove with conan 2.0)
        self.cpp_info.components[libsw_sysemu_comp_name].set_property("cmake_target_name", libsw_sysemu_cmake_name)
        self.cpp_info.components[libsw_sysemu_comp_name].requires = [libsysemu_comp_name, libfpu_comp_name, "glog::glog"]
        self.cpp_info.components[libsw_sysemu_comp_name].libs = ["sw-sysemu"]
        self.cpp_info.components[libsw_sysemu_comp_name].includedirs =  ["include"]
        self.cpp_info.components[libsw_sysemu_comp_name].libdirs = ["lib"]

        # utilities
        bin_path = os.path.join(self.package_folder, "bin")
        bin_ext = ".exe" if self.settings.os == "Windows" else ""

        self.output.info("Appending PATH env var with : {}".format(bin_path))
        self.env_info.PATH.append(bin_path)

        sys_emu = os.path.join(self.package_folder, "bin", "sys_emu" + bin_ext)
        self.output.info("Setting SYS_EMU to {}".format(sys_emu))
        self.env_info.SYS_EMU = sys_emu
