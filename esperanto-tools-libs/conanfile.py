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
        "with_tools": [True, False],
        "with_tests": [True, False],
        "with_sysemu_artifacts": ['unreleased', 'latest'],
    }
    default_options = {
        "with_tools": False,
        "with_tests": False,
        "with_sysemu_artifacts": "unreleased",
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/esperanto-tools-libs.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"

    python_requires = "conan-common/[>=0.5.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, self.name)

    def configure_options(self):
        if self.options.with_tests and not self.dependencies["esperanto-flash-tool"].options.get_safe("header_only"):
            raise ConanInvalidConfiguration("When enabling runtime tests esperanto-flash-tool:header_only must be True")

    @property
    def _min_device_api(self):
        return {
            "unreleased": "1.0.0",
            "latest": "1.0.0",
        }.get(str(self.options.with_sysemu_artifacts))

    def requirements(self):
        self.requires(f"deviceApi/{self._min_device_api}")
        self.requires("deviceLayer/[>=1.1.0 <2.0.0]")
        self.requires("hostUtils/[>=0.1.0 <1.0.0]")

        self.requires("cereal/1.3.1")
        self.requires("elfio/3.8")
        self.requires("libcap/2.62")
        self.requires("gflags/2.2.2")

        self.requires("cmake-modules/[>=0.4.1 <1.0.0]")

        if self.options.with_tests:
            self.requires("gtest/1.10.0")
            self.requires("sw-sysemu/0.5.0")

            sysemu_artifacts_conanfile = f"conanfile_device_artifacts_{self.options.with_sysemu_artifacts}.txt"
            self.run(f"conan install {sysemu_artifacts_conanfile} -pr:b default -pr:h baremetal-rv64-gcc8.2-release --remote conan-develop --build missing")

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
            self.run("ctest -L 'Generic' --no-compress-output")

    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        # library components
        self.cpp_info.components["etrt"].names["cmake_find_package"] = "etrt"
        self.cpp_info.components["etrt"].names["cmake_find_package_multi"] = "etrt"
        self.cpp_info.components["etrt"].set_property("cmake_target_name", "runtime::etrt")
        self.cpp_info.components["etrt"].requires = ["hostUtils::debug", "deviceApi::deviceApi", "libcap::libcap", "cereal::cereal", "deviceLayer::deviceLayer", "hostUtils::logging", "hostUtils::threadPool", "elfio::elfio"]
        self.cpp_info.components["etrt"].libs = ["etrt"]
        self.cpp_info.components["etrt"].includedirs = ["include"]
        self.cpp_info.components["etrt"].libdirs = ["lib"]

        self.cpp_info.components["etrt_static"].names["cmake_find_package"] = "etrt_static"
        self.cpp_info.components["etrt_static"].names["cmake_find_package_multi"] = "etrt_static"
        self.cpp_info.components["etrt_static"].set_property("cmake_target_name", "runtime::etrt_static")
        self.cpp_info.components["etrt_static"].requires = ["hostUtils::debug", "deviceApi::deviceApi", "libcap::libcap", "cereal::cereal", "deviceLayer::deviceLayer", "hostUtils::logging", "hostUtils::threadPool", "elfio::elfio"]
        self.cpp_info.components["etrt_static"].libs = ["etrt_static"]
        self.cpp_info.components["etrt_static"].includedirs = ["include"]
        self.cpp_info.components["etrt_static"].libdirs = ["lib"]
        
        binpath = os.path.join(self.package_folder, "bin")
        self.output.info("Appending PATH env var: {}".format(binpath))
        self.runenv_info.prepend_path('PATH', binpath)

        # TODO: to remove in conan v2 once old virtualrunenv is removed
        self.env_info.PATH.append(binpath)
