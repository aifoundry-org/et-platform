from conan import ConanFile
from conan.tools.cmake import CMake
from conan.tools.build import can_run
import os

class LoggingTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    test_type = "explicit"

    def requirements(self):
        self.requires(self.tested_reference_str)
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            for test_package in ["test_package_static", "test_package_shared"]:
                bin_path = os.path.join("bin", test_package)
                self.run(bin_path, env="conanrun")
