from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conans import tools
from conans.errors import ConanInvalidConfiguration
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
        "url": "git@gitlab.esperanto.ai:software/firmware-flash-tool.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"

    python_requires = "conan-common/[>=0.5.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, "EsperantoFlashTool")

    def requirements(self):
        if not self.options.header_only:
            self.requires("json-c/0.15")
    
    def package_id(self):
        if self.options.header_only:
            self.info.header_only()

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
            tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "EsperantoFlashTool")
        self.cpp_info.set_property("cmake_target_name", "esperanto-flash-tool::headers")

        self.cpp_info.includedirs.append(os.path.join("include", "esperanto", "flash-tool"))
