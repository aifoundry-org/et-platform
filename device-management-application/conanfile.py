from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import rmdir
import os

required_conan_version = ">=1.52.0"


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

    python_requires = "conan-common/[>=0.1.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, self.name)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
    
    def configure(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")
    
    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("deviceManagement/[>=0.6.0 <1.0.0]")
        self.requires("esperantoTrace/[>=0.6.0 <1.0.0]")
        self.requires("deviceLayer/[>=1.1.0 <2.0.0]")
        self.requires("hostUtils/[>=0.1.0 <1.0.0]")

        self.requires("fmt/7.1.3")
        self.requires("glog/0.4.0")
    
    def generate(self):
        tc = CMakeToolchain(self)
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
        cmake.configure()
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        # this pkg only contains executables
        self.cpp_info.frameworkdirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.resdirs = []
        self.cpp_info.includedirs = []

        # TODO: Legacy, to be removed on Conan 2.0
        bin_folder = os.path.join(self.package_folder, "bin")
        self.env_info.PATH.append(bin_folder)
