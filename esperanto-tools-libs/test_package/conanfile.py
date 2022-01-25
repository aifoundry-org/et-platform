from conan import ConanFile
from conan.tools.cmake import CMake
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
        if not tools.cross_building(self):
            for test_package in ["test_package_static", "test_package_shared"]:
                bin_path = os.path.join("bin", test_package)
                self.run(bin_path, run_environment=True)