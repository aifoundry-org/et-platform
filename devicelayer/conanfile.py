from conan import ConanFile
from conan.tools.build import cross_building
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import get, rmdir
import os

required_conan_version = ">=1.52.0"


class DeviceLayerConan(ConanFile):
    name = "deviceLayer"
    url = "git@gitlab.com:esperantotech/software/devicelayer.git"
    homepage = "https://gitlab.com/esperantotech/software/devicelayer"
    description = ""
    license = "Esperanto Technologies"

    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "fvisibility": ["default", "protected", "hidden"],
        "with_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "fvisibility": "default",
        "with_tests": False,
    }

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"
        
    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)
    
    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)

    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def config_options(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        # public dependencies
        self.requires("sw-sysemu/0.20.0-alpha")
        self.requires("et-host-utils/0.4.0-alpha")
        self.requires("gtest/1.10.0")  # Required in public-interface header: IDeviceLayerMock.h
        # private dependencies
        self.requires("linuxDriver/0.15.0")
        self.requires("boost/1.72.0")  # TODO we should not depend on boost

    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.4.1 <1.0.0]")
        self.tool_requires("cmake/[>=3.21 <4]")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def layout(self):
        cmake_layout(self)

        self.cpp.package.includedirs = ["include"]
        self.cpp.package.libs = ["device-layer"]
        self.cpp.package.requires = [
            # IDeviceLayer.h
            "sw-sysemu::sw-sysemu",
            "hostUtils::debug",
            # IDeviceLayerMock.h
            "gtest::gmock",

            # device-layer private
            "hostUtils::logging",
            "linuxDriver::linuxDriver",
            "boost::boost"
        ]
        self.cpp.build.includedirs = ["include"]
        self.cpp.build.libs = ["device-layer"]
        self.cpp.source.includedirs = ["include"]

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()
        if not cross_building(self):
            env = VirtualRunEnv(self)
            env.generate(scope="build")

        tc = CMakeToolchain(self)
        tc.variables["ENABLE_TESTS"] = self.options.get_safe("with_tests")
        tc.variables["BUILD_DOCS"] = False
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
        tc.variables["CMAKE_ASM_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_C_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_CXX_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_VISIBILITY_INLINES_HIDDEN"] = "ON" if self.options.fvisibility == "hidden" else "OFF"
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
        self.cpp_info.set_property("cmake_file_name", "deviceLayer")
        self.cpp_info.set_property("cmake_target_name", "deviceLayer::deviceLayer")
