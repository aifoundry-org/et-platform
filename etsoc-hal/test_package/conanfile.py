from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.build import can_run
import os


class EtsocHalTestConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    test_type = "explicit"

    def requirements(self):
        self.requires(self.tested_reference_str)
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            bin_path = os.path.join("bin", "test_package")
            self.run(bin_path, env="conanrun")
