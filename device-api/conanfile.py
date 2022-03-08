from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conans import tools
import os
import re
import sys
import subprocess

class DeviceApiConan(ConanFile):
    name = "deviceApi"
    url = "https://gitlab.esperanto.ai/software/device-api.git"
    license = "Esperanto Technologies"
    
    settings = "os", "compiler", "arch", "build_type"

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/device-api.git",
        "revision": "auto",
    }
    generator = "CMakeDeps"

    python_requires = "conan-common/[>=0.5.0 <1.0.0]"


    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, "deviceApi")

    def generate(self):
        tc = CMakeToolchain(self)
        # not including documentation for now
        tc.variables["DEVICEAPI_BUILD_DOC"] = False
        tc.generate()

    def build(self):
        # install python requirements
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
        
    def package_id(self):
        self.info.header_only()
    
    def package_info(self):
        self.cpp_info.includedirs.append(os.path.join("include", "esperanto"))
