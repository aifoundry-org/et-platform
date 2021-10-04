from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
import os

class DeviceApiConan(ConanFile):
    name = "deviceApi"
    version = "0.1.0"
    
    settings = "os", "compiler", "arch", "build_type"
    exports_sources = "cmake/*", "src/*", "CMakeLists.txt", "deviceApiConfig.cmake.in"

    @property
    def _build_subfolder(self):
        return "build_subfolder"
 
    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    _cmake = None
    def _configure_cmake(self):
        if not self._cmake:
            cmake = CMake(self)
            cmake.configure()
            self._cmake = cmake
        return self._cmake
    
    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
    
    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        
    def package_id(self):
        self.info.header_only()
    
    def package_info(self):
        self.cpp_info.includedirs.append(os.path.join("include", "esperanto"))
