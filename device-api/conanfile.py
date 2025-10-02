from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import rmdir
import os

required_conan_version = ">=1.59.0"


class DeviceApiConan(ConanFile):
    name = "deviceApi"
    url = "git@gitlab.com:esperantotech/software/device-api.git"
    homepage = "https://gitlab.com/esperantotech/software/device-api"
    license = "Esperanto Technologies"
    
    settings = "os", "compiler", "arch", "build_type"

    python_requires = "conan-common/[>=1.1.1 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)

    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)
    
    def layout(self):
        cmake_layout(self)
    
    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def package_id(self):
        self.info.clear()
    
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
        
    def package_info(self):
        self.cpp_info.includedirs.append(os.path.join("include", "esperanto"))
