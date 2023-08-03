from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import get, rmdir
from conan.tools.scm import Version
import os

required_conan_version = ">=1.52.0"


class DeviceLayerConan(ConanFile):
    name = "deviceLayer"
    url = "git@gitlab.com:esperantotech/software/devicelayer.git"
    homepage = "https://gitlab.com/esperantotech/software/devicelayer"
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

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"
        
    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)
    
    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)

    def layout(self):
        cmake_layout(self)
    
    def requirements(self):
        self.requires("sw-sysemu/0.12.0")
        self.requires("hostUtils/0.3.0")
        self.requires("linuxDriver/0.15.0")
        self.requires("boost/1.72.0")

        # IDeviceLayerMock.h
        self.requires("gtest/1.10.0")
    
    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.4.1 <1.0.0]")

    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ENABLE_TESTS"] = self.options.get_safe("with_tests")
        tc.variables["ENABLE_DEPRECATED"] = False
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.variables["BUILD_DOCS"] = False
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
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
        self.cpp_info.libs = ["deviceLayer"]
        self.cpp_info.requires = [
            # IDeviceLayer.h
            "sw-sysemu::sw-sysemu",
            "hostUtils::debug",
            # IDeviceLayerMock.h
            "gtest::gmock",

            # deviceLayer private
            "hostUtils::logging",
            "linuxDriver::linuxDriver",
            "boost::boost"
        ]
        if not self.options.shared:
            if self.settings.compiler == "gcc" and Version(str(self.settings.compiler.version)) < "9":
                self.cpp_info.system_libs.append("stdc++fs")
