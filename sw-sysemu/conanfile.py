from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
from conans.errors import ConanInvalidConfiguration
import os
import shutil

class SwSysemuConan(ConanFile):
    name = "sw-sysemu"
    version = "1.0"
    url = "https://gitlab.esperanto.ai/software/sw-sysemu"
    description = "The functional ETSOC-1 emulator. Able to execute RISC-V instructions with Esperanto extensions"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "profiling": [True, False],
        "backtrace": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "profiling": False,
        "backtrace": False
    }

    generators = "cmake_find_package_multi"

    #TODO: Ideally we should have all sources inside 'src'
    exports_sources = [
        "CMakeLists.txt",
        "*.h",
        "*.cpp",
        "devices/*",
        "fpu/*",
        "insns/*",
        "memory/*",
        "softfloat/*",
        "sw-sysemu/*",
        "sys_emu/*",
        "sw-sysemuConfig.cmake.in"
    ]

    requires = [
        "glog/0.4.0",
        "elfio/3.8"
    ]
    python_requires = "conan-common/0.1.0"

    def requirements(self):
        if self.options.backtrace:
            self.requires("libunwind/1.5.0")
    
    @property
    def _build_subfolder(self):
        return "build_subfolder"
        
    def configure(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["PROFILING"] = self.options.profiling
        tc.variables["BACKTRACE"] = self.options.backtrace
        tc.generate()

    _cmake = None
    def _configure_cmake(self):
        if not self._cmake:
            cmake = CMake(self, build_folder=self._build_subfolder)
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
        # library components
        self.cpp_info.components["libsysemu"].names["cmake_find_package"] = "sysemu"
        self.cpp_info.components["libsysemu"].names["cmake_find_package_multi"] = "sysemu"
        self.cpp_info.components["libsysemu"].requires = ["elfio::elfio"]
        if self.options.backtrace:
            self.cpp_info.components["libsysemu"].requires.append("libunwind::libunwind")
        
        self.cpp_info.components["libfpu"].names["cmake_find_package"] = "fpu"
        self.cpp_info.components["libfpu"].names["cmake_find_package_multi"] = "fpu"
        self.cpp_info.components["libfpu"].requires = []

        self.cpp_info.components["libsw-sysemu"].names["cmake_find_package"] = "sw-sysemu"
        self.cpp_info.components["libsw-sysemu"].names["cmake_find_package_multi"] = "sw-sysemu"
        self.cpp_info.components["libsw-sysemu"].requires = ["libsysemu", "libfpu", "glog::glog"]

        # utilities
        bin_path = os.path.join(self.package_folder, "bin")
        bin_ext = ".exe" if self.settings.os == "Windows" else ""

        self.output.info("Appending PATH env var with : {}".format(bin_path))
        self.env_info.PATH.append(bin_path)

        sys_emu = tools.unix_path(os.path.join(self.package_folder, "bin", "sys_emu" + bin_ext))
        self.output.info("Setting SYS_EMU to {}".format(sys_emu))
        self.env_info.SYS_EMU = sys_emu
