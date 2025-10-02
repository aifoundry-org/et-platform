from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import rmdir
from conan.tools.scm import Git
import os

required_conan_version = ">=1.59.0"


class EtsocHalConan(ConanFile):
    name = "etsoc_hal"
    url = "git@gitlab.com:esperantotech/software/etsoc_hal.git"
    homepage = "https://gitlab.com/esperantotech/software/etsoc_hal"
    description = "Etsoc_hal is designed to be a bridge between RTL and system software." \
                  "It contains not only header files but also algorithm suggested by RTL team"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {}
    default_options = {}


    python_requires = "conan-common/[>=1.1.2 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)
    
    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        # etsoc_hal should not require anything
        pass

    def package_id(self):
        self.python_requires["conan-common"].module.x86_64_compatible(self)
    
    def validate(self):
        if self.settings.arch != "rv64":
            raise ConanInvalidConfiguration("Cross-compiling to arch %s is not supported" % self.settings.arch)
    
    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
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
        self.cpp_info.libs = ["etsoc_hal"]
        self.cpp_info.includedirs = [
            os.path.join("include", "esperanto-fw"),
            os.path.join("include", "esperanto-fw", "etsoc_hal")
        ]
        self.cpp_info.libdirs = [os.path.join("lib", "esperanto-fw")]
