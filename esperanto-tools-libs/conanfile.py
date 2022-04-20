from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.layout import cmake_layout
from conans import tools
from conans.errors import ConanInvalidConfiguration
import os
import re


class RuntimeConan(ConanFile):
    name = "runtime"
    url = "https://gitlab.esperanto.ai/software/esperanto-tools-libs"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "with_tests": [True, False],
        "with_tools": [True, False],
    }
    default_options = {
        "with_tests": False,
        "with_tools": False
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/esperanto-tools-libs.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"

    python_requires = "conan-common/[>=0.5.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, "runtime")

    def configure_options(self):
        if self.options.with_tests and not self.dependencies["esperanto-flash-tool"].options.get_safe("header_only"):
            raise ConanInvalidConfiguration("When enabling runtime tests esperanto-flash-tool:header_only must be True")
    
    def requirements(self):
        if self.options.with_tests:
            self.requires("deviceApi/0.4.0#b581ee71930e41a44a3e10d4ff40496e")
        else:
            self.requires("deviceApi/0.3.0")
        self.requires("deviceLayer/0.1.0")
        self.requires("hostUtils/0.1.0")
        
        self.requires("cereal/1.3.1")
        self.requires("elfio/3.8")
        
        self.requires("cmake-modules/[>=0.4.1 <1.0.0]")

        if self.options.with_tests:
            self.requires("sw-sysemu/0.2.0")

            self.requires("et-common-libs/0.0.5@#44d0cbac248a5fe6b3f7a20746bbe80c")
            self.requires("device-minion-runtime/0.0.4@#29e324c4bfdae4f81c4c7ed747a719a2")
            self.requires("device-bootloaders/0.2.0@#5f32a430da600817e7532f007218c416")
            self.requires("esperanto-test-kernels/0.1.0@#a943278128156b0c567a9851f89745b9")

            # only for pinning dependencies
            self.requires("esperantoTrace/0.1.0#2c4f00fce55cebb9fb057bc474da499c")
            self.requires("etsoc_hal/0.1.0#35173765483f78347d3b0d2e50d78a44")

    
    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def layout(self):
        cmake_layout(self)
        self.folders.source = "."
    
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.get_safe("with_tests")
        tc.variables["BUILD_TOOLS"] = self.options.get_safe("with_tools")
        tc.variables["BUILD_DOCS"] = False
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.deps_cpp_info["cmake-modules"].rootpath, "cmake")
        tc.generate()
        
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.with_tests and not tools.cross_building(self.settings):
            self.run("ctest -T Test -L 'Generic' --no-compress-output")

    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        # library components
        self.cpp_info.components["etrt"].names["cmake_find_package"] = "etrt"
        self.cpp_info.components["etrt"].names["cmake_find_package_multi"] = "etrt"
        self.cpp_info.components["etrt"].set_property("cmake_target_name", "runtime::etrt")
        self.cpp_info.components["etrt"].requires = ["hostUtils::debug", "cereal::cereal", "deviceApi::deviceApi", "deviceLayer::deviceLayer", "hostUtils::logging", "elfio::elfio"]
        self.cpp_info.components["etrt"].libs = ["etrt"]
        self.cpp_info.components["etrt"].includedirs = ["include"]
        self.cpp_info.components["etrt"].libdirs = ["lib"]

        self.cpp_info.components["etrt_static"].names["cmake_find_package"] = "etrt_static"
        self.cpp_info.components["etrt_static"].names["cmake_find_package_multi"] = "etrt_static"
        self.cpp_info.components["etrt_static"].set_property("cmake_target_name", "runtime::etrt_static")
        self.cpp_info.components["etrt_static"].requires = ["hostUtils::debug", "cereal::cereal", "deviceApi::deviceApi", "deviceLayer::deviceLayer", "hostUtils::logging", "elfio::elfio"]
        self.cpp_info.components["etrt_static"].libs = ["etrt_static"]
        self.cpp_info.components["etrt_static"].includedirs = ["include"]
        self.cpp_info.components["etrt_static"].libdirs = ["lib"]
