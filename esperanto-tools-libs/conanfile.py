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
        device_api = "deviceApi/0.5.0@"
        device_layer = "deviceLayer/0.3.0@"
        if self.options.with_tests:
            device_api += "#5859dbf139ef47467a1d6a664c33d107"
            device_layer += "#1d2eb12464a109ee1ba83a38bd4e1f6a"
        self.requires(device_api)
        self.requires(device_layer)
        self.requires("hostUtils/0.1.0")

        self.requires("cereal/1.3.1")
        self.requires("elfio/3.8")

        self.requires("cmake-modules/[>=0.4.1 <1.0.0]")

        if self.options.with_tests:
            self.requires("gtest/1.10.0")
            self.requires("sw-sysemu/0.2.0")

            self.requires("et-common-libs/0.7.0@#1f3ab13ae2efbbbe188e6b4d6a362fe0")
            self.requires("device-minion-runtime/0.7.0@#d962971c84898cab59f821d62df82ad1")
            self.requires("device-bootloaders/0.3.0@#07bafe92cd37a7229e986010349bad73")
            self.requires("esperanto-test-kernels/1.0.0@#37beed210aa1606b19c74957f64d475e")

            # only for pinning dependencies
            self.requires("esperantoTrace/0.5.0@#ac7a9098e0140c7858cbe75ee569a079")
            self.requires("etsoc_hal/1.0.0@#f53ecff2c8a176f37f9e3379d0e19395")
            self.requires("tf-protocol/0.1.0@#87d3b8e7ad2f0b39fa6fae35f3bc180b")
            self.requires("signedImageFormat/1.0@#4503615bd9e6ca9cfae2441dddb96b2e")
            self.requires("esperanto-flash-tool/1.0.0@#a3734839a8d548175ca1c17bb2b502aa")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def layout(self):
        cmake_layout(self)
        self.folders.source = "."

    def generate(self):
        device_api = self.dependencies["deviceApi"]
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
