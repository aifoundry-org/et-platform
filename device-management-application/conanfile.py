from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.layout import cmake_layout
from conans.errors import ConanInvalidConfiguration
import os

class DeviceManagementApplicationConan(ConanFile):
    name = "deviceManagementApplication"
    url = "https://gitlab.esperanto.ai/software/device-management-application"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "fPIC": [True, False],
    }
    default_options = {
        "fPIC": True,
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/device-management-application.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"

    python_requires = "conan-common/[>=0.1.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, self.name)

    def configure(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")
    
    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("deviceManagement/[>=0.6.0 <1.0.0]")
        self.requires("esperantoTrace/[>=0.6.0 <1.0.0]")
        self.requires("deviceLayer/[>=0.4.0 <1.0.0]")
        self.requires("hostUtils/[>=0.1.0 <1.0.0]")
        self.requires("deviceApi/[>=0.7.0 <1.0.0]")

        self.requires("fmt/7.1.3")
        self.requires("glog/0.4.0")
    
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.variables["CMAKE_VERBOSE_MAKEFILE"] = True
        tc.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)

        # utilities
        bin_path = os.path.join(self.package_folder, "bin")

        self.output.info("Appending PATH env var with : {}".format(bin_path))
        self.runenv_info.append("PATH", bin_path)
        self.env_info.PATH.append(bin_path) # deprecated remove in Conan 2.0.0
