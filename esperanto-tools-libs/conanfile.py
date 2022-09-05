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
        device_api = "deviceApi/0.6.0@"
        device_layer = "deviceLayer/[>=0.3.0 <1.0.0]@"
        if self.options.with_tests:
            device_api += "#3e1a8064f37596b34be470b368556536"
            device_layer += "#1d2eb12464a109ee1ba83a38bd4e1f6a"
        self.requires(device_api)
        self.requires(device_layer)
        self.requires("hostUtils/[>=0.1.0 <1.0.0]")

        self.requires("cereal/1.3.1")
        self.requires("elfio/3.8")
        self.requires("libcap/2.62")
        self.requires("gflags/2.2.2")

        self.requires("cmake-modules/[>=0.4.1 <1.0.0]")

        if self.options.with_tests:
            self.requires("gtest/1.10.0")
            self.requires("sw-sysemu/0.2.0")

            self.requires("et-common-libs/0.9.0@#330284afd56b61ddd4c1b1dfde597b53")
            self.requires("device-minion-runtime/0.10.0#4bded86c8893468057a7600258151b23")
            self.requires("device-bootloaders/0.4.0#d6798104317f163cbe860cf5c9c11f5a")
            self.requires("esperanto-test-kernels/1.2.0@#c65c500cbea0795566249e17fa3bfa1c")

            # only for pinning dependencies
            self.requires("esperantoTrace/0.6.0#c24ff325c75833a83a04f13414a1f10e")
            self.requires("etsoc_hal/1.0.0@#f53ecff2c8a176f37f9e3379d0e19395")
            self.requires("tf-protocol/0.2.0#fe6749a6d3e624d1d211b475e122880f")
            self.requires("signedImageFormat/1.0#8b0007bbb87386fd90730d7e1aeb4089")
            self.requires("esperanto-flash-tool/1.1.0#287ea2a3bf61faa862bea0083aaa5c7f")

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
            self.run("sudo ctest -T Test -L 'Generic' --no-compress-output")

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
