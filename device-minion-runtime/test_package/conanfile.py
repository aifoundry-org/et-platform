from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conans import tools
import os

class DeviceMinionRuntimeTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def test(self):
        pass
