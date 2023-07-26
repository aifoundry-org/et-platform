from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conans import tools 
import os 
import re
import sys
import subprocess

class TfProtocolCOnan(ConanFile):
    name = "tf-protocol"
    url = "git@gitlab.com:esperantotech/software/tf-protocol.git"
    homepage = "https://gitlab.com/esperantotech/software/tf-protocol.git"
    license = "Esperanto Technologies"

    settings = "os", "compiler", "arch", "build_type"

    generator = "CMakeDeps"

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"
    
    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, self.name)

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

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", os.path.join(self.source_folder, "requirements.txt")])
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
