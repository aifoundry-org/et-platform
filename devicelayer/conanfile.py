from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.layout import cmake_layout
from conans import tools
import os
import re


class DeviceLayerConan(ConanFile):
    name = "deviceLayer"
    url = "https://gitlab.esperanto.ai/software/deviceLayer"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_tests": False,
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/devicelayer.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"


    python_requires = "conan-common/[>=0.1.0 <1.0.0]"
        
    def set_version(self):
        content = tools.load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
        version = re.search(r"set\(PROJECT_VERSION (.*)\)", content).group(1)
        self.version = version.strip()
    
    def requirements(self):
        self.requires("sw-sysemu/0.2.0")
        self.requires("hostUtils/0.1.0")
        self.requires("linuxDriver/0.1.0")
        self.requires("boost/1.72.0")

        # IDeviceLayerFake.h
        self.requires("deviceApi/0.1.0")
        # IDeviceLayerMock.h
        self.requires("gtest/1.8.1")
        
        self.requires("cmake-modules/[>=0.4.1 <1.0.0]")
    
    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")
        
    def layout(self):
        cmake_layout(self)
        self.folders.source = "."

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ENABLE_TESTS"] = self.options.get_safe("with_tests")
        tc.variables["ENABLE_DEPRECATED"] = False
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies["cmake-modules"].package_folder, "cmake")
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
        self.cpp_info.libs = tools.collect_libs(self)
        self.cpp_info.requires = [
            # IDeviceLayer.h
            "sw-sysemu::sw-sysemu",
            "hostUtils::debug",
            # IDeviceLayerFake.h
            "deviceApi::deviceApi",
            # IDeviceLayerMock.h
            "gtest::gmock",

            # deviceLayer private
            "hostUtils::logging",
            "linuxDriver::linuxDriver",
            "boost::boost"
        ]
        if not self.options.shared:
            if self.settings.compiler == "gcc" and tools.Version(self.settings.compiler.version) < "9":
                self.cpp_info.system_libs.append("stdc++fs")
