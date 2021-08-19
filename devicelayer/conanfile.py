from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.layout import cmake_layout
from conans.errors import ConanInvalidConfiguration
import os
class DeviceLayerConan(ConanFile):
    name = "deviceLayer"
    version = "0.1.0"
    url = "https://gitlab.esperanto.ai/software/deviceLayer"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_fake_interface": [True, False],
        "with_mock_interface": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_fake_interface": False,
        "with_mock_interface": False
    }

    generators = "cmake_find_package_multi"

    exports_sources = [ "CMakeLists.txt", "cmake/*", "include/*", "src/*", "DeviceLayerConfig.cmake.in" ]

    requires = [ 
        "sw-sysemu/0.2.0",
        "hostUtils/0.1.0",
        "linuxDriver/0.1.0",
        "esperantoTrace/0.1.0",
        "boost/1.72.0"
    ]
    build_requires = "cmake-modules/[>=0.4.1 <1.0.0]"
    python_requires = "conan-common/[>=0.1.0 <1.0.0]"
    
    def configure(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def requirements(self):
        if self.options.get_safe("with_fake_interface"):
            self.requires("deviceApi/0.1.0")
        if self.options.get_safe("with_mock_interface"):
            self.requires("gtest/1.8.1")
        
    def layout(self):
        cmake_layout(self)
        self.folders.source = "."

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ENABLE_TESTS"] = False
        tc.variables["ENABLE_DEPRECATED"] = False
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.deps_cpp_info["cmake-modules"].rootpath, "cmake")
        tc.generate()
    
    _cmake = None
    def _configure_cmake(self):
        if not self._cmake:
            cmake = CMake(self)
            cmake.configure()
            self._cmake = cmake
        return self._cmake
    
    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
    
    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        self.cpp_info.requires = [
            # deviceLayer public
            "sw-sysemu::sw-sysemu",
            "hostUtils::debug",
            # deviceLayer private
            "hostUtils::logging",
            "linuxDriver::linuxDriver",
            "boost::boost"
        ]
        if self.options.get_safe("with_fake_interface"):
            self.cpp_info.requires.append("deviceApi::deviceApi")
        if self.options.get_safe("with_mock_interface"):
            self.cpp_info.requires.append("gtest::gmock")
