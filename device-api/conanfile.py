from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import rmdir
import os


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

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)

    def generate(self):
        tc = CMakeToolchain(self)
        # not including documentation for now
        tc.variables["DEVICEAPI_BUILD_DOC"] = False
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        
    def package_id(self):
        self.info.header_only()
    
    def package_info(self):
        self.cpp_info.includedirs.append(os.path.join("include", "esperanto"))
