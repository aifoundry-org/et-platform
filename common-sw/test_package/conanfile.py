from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conans import tools
import os

class LoggingTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not tools.cross_building(self.settings):
            for test_package in ["test_package_debug", "test_package_logging", "test_package_threadpool", "test_package_all"]:
                bin_path = os.path.join("bin", test_package)
                self.run(bin_path, run_environment=True)