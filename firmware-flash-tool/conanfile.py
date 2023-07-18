from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import rmdir
import os


class EsperantoFlashToolConan(ConanFile):
    name = "esperanto-flash-tool"
    description = "esperanto flashing tool"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "header_only": [True, False],
    }
    default_options = {
        "header_only": False,
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.com:esperantotech/software/firmware-flash-tool.git",
        "revision": "auto",
    }

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, "EsperantoFlashTool")

    def requirements(self):
        if not self.options.header_only:
            self.requires("json-c/0.15")

    def package_id(self):
        if self.options.header_only:
            self.info.clear()

    def validate(self):
        if self.settings.arch == "rv64" and not self.options.header_only:
            raise ConanInvalidConfiguration("When cross-compiling to arch={} only "
                                            "{}::header_only=True is allowed."
                                            .format(self.settings.arch, self.name))

    def generate(self):
        if not self.options.header_only:
            tc = CMakeToolchain(self)
            tc.variables["ENABLE_WARNINGS_AS_ERRORS"] = False
            tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
            tc.generate()

            deps = CMakeDeps(self)
            deps.generate()

    def build(self):
        if not self.options.header_only:
            cmake = CMake(self)
            cmake.configure()
            cmake.build()

    def package(self):
        if self.options.header_only:
            # headers used in some RISC-V projects
            self.copy("esperanto_flash_image.h", dst=os.path.join("include", "esperanto", "flash-tool"), src=os.path.join(self.source_folder, "include"))
        else:
            cmake = CMake(self)
            cmake.install()
            rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "EsperantoFlashTool")
        self.cpp_info.set_property("cmake_target_name", "esperanto-flash-tool::headers")

        self.cpp_info.includedirs.append(os.path.join("include", "esperanto", "flash-tool"))
