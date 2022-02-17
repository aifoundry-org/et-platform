from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conans import tools
from conans.errors import ConanInvalidConfiguration
import os
import re


class EsperantoTestKenelsConan(ConanFile):
    name = "esperanto-test-kernels"
    description = "Suite of test kernels"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {}
    default_options = {}

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/test-compute-kernels.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"

    def set_version(self):
        content = tools.load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
        version = re.search(r"project\(EsperantoTestKernels VERSION \s*([\d.]+)", content).group(1)
        self.version = version.strip()

    def requirements(self):
        self.requires("esperantoTrace/0.1.0")
        self.requires("et-common-libs/0.0.1")

    def validate(self):
        if self.settings.arch != "rv64":
            raise ConanInvalidConfiguration("Cross-compiling to arch {} is not supported".format(self.settings.arch))

        et_common_libs = self.dependencies["et-common-libs"]
        # et-common-libs must be compiled with these components
        for flag in ["with_cm_umode"]:
            if not et_common_libs.options.get_safe(flag):
                raise ConanInvalidConfiguration("{0} requires {1} package with '-o {1}:{2}'".format(self.name, "et-common-libs", flag))

    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.5.0 <1.0.0]")

    def generate(self):
        # Get the toolchains from "tools.cmake.cmaketoolchain:user_toolchain" conf at the
        # tool_requires
        user_toolchains = []
        for dep in self.dependencies.direct_build.values():
            ut = dep.conf_info["tools.cmake.cmaketoolchain:user_toolchain"]
            if ut:
                user_toolchains.append(ut)
        
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"

        if user_toolchains:
            self.output.info("Applying user_toolchains: %s" % user_toolchains)
            tc.blocks["user_toolchain"].values["paths"] = user_toolchains
        
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
    