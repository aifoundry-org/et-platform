from conan import ConanFile

from conan.tools.build import can_run
from conan.tools.cmake import cmake_layout, CMake
import os


class LoggingTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"
    test_type = "explicit"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            for test_package in ["test_package_debug", "test_package_logging", "test_package_threadpool", "test_package_all"]:
                bin_path = os.path.join("bin", test_package)
                self.run(bin_path, env="conanrun")