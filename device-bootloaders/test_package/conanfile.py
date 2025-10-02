from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
import os
import glob

class EsperantoBootLoadersTestConan(ConanFile):
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
        # we don't build any test, no need to execute anything
        pass
        
