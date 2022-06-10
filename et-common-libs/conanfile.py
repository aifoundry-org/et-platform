from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conans import tools
from conans.errors import ConanInvalidConfiguration
import os
import re

required_conan_version = ">=1.46.0"

class EtCommonLibsConan(ConanFile):
    name = "et-common-libs"
    url = "https://gitlab.esperanto.ai/software/et-common-libs.git"
    description = "Esperanto common libraries - used across SP, MM MMode, MM SMode, CM MMode, CM SMode, CM UMode."
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "warnings_as_errors": [True, False],
        "with_sp_bl": [True, False],
        "with_cm_umode": [True, False],
        "with_minion_bl": [True, False],
        "with_mm_rt_svcs": [True, False],
        "with_cm_rt_svcs": [True, False],
    }
    default_options = {
        "warnings_as_errors": True,
        "with_sp_bl": True,
        "with_cm_umode": True,
        "with_minion_bl": True,
        "with_mm_rt_svcs": True,
        "with_cm_rt_svcs": True,
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/et-common-libs.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"

    python_requires = "conan-common/[>=0.5.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, "et-common-libs")

    def configure(self):
        # et-common-libs is a C library, doesn't depend on any C++ standard library
        del self.settings.compiler.libcxx
        del self.settings.compiler.cppstd

    def requirements(self):
        self.requires("esperantoTrace/0.5.0")
        # cm-umode doens't require etsoc_hal
        if self.options.with_sp_bl or \
           self.options.with_minion_bl or \
           self.options.with_mm_rt_svcs or \
           self.options.with_cm_rt_svcs:
           self.requires("etsoc_hal/0.1.0")

    def package_id(self):
        self.python_requires["conan-common"].module.x86_64_compatible(self)

    def validate(self):
        if self.settings.arch != "rv64":
            raise ConanInvalidConfiguration("Cross-compiling to arch %s is not supported" % self.settings.arch)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ENABLE_WARNINGS_AS_ERRORS"] = self.options.warnings_as_errors
        tc.variables["WITH_SP_BL"] = self.options.with_sp_bl
        tc.variables["WITH_CM_UMODE"] = self.options.with_cm_umode
        tc.variables["WITH_MINION_BL"] = self.options.with_minion_bl
        tc.variables["WITH_MM_RT_SVCS"] = self.options.with_mm_rt_svcs
        tc.variables["WITH_CM_RT_SVCS"] = self.options.with_cm_rt_svcs
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):

        if self.options.with_sp_bl:
            self.cpp_info.components["sp-bl1"].set_property("cmake_target_name", "et-common-libs::sp-bl1")
            self.cpp_info.components["sp-bl1"].includedirs = [os.path.join("sp-bl1", "include")]
            self.cpp_info.components["sp-bl1"].libdirs = [os.path.join("sp-bl1", "lib")]
            self.cpp_info.components["sp-bl1"].libs = ["sp-bl1"]
            self.cpp_info.components["sp-bl1"].requires = ["etsoc_hal::etsoc_hal"]

            self.cpp_info.components["sp-bl2"].set_property("cmake_target_name", "et-common-libs::sp-bl2")
            self.cpp_info.components["sp-bl2"].includedirs = [os.path.join("sp-bl2", "include")]
            self.cpp_info.components["sp-bl2"].libdirs = [os.path.join("sp-bl2", "lib")]
            self.cpp_info.components["sp-bl2"].libs = ["sp-bl2"]
            self.cpp_info.components["sp-bl2"].defines = ["SP_RT=1", "SERVICE_PROCESSOR_BL2=1"]
            self.cpp_info.components["sp-bl2"].requires = ["etsoc_hal::etsoc_hal", "esperantoTrace::et_trace"]

        if self.options.with_cm_umode:
            self.cpp_info.components["cm-umode"].set_property("cmake_target_name", "et-common-libs::cm-umode")
            self.cpp_info.components["cm-umode"].includedirs = [os.path.join("cm-umode", "include")]
            self.cpp_info.components["cm-umode"].libdirs = [os.path.join("cm-umode", "lib")]
            self.cpp_info.components["cm-umode"].libs = ["cm-umode"]
            self.cpp_info.components["cm-umode"].requires = ["esperantoTrace::et_trace"]

        if self.options.with_minion_bl:
            self.cpp_info.components["minion-bl"].set_property("cmake_target_name", "et-common-libs::minion-bl")
            self.cpp_info.components["minion-bl"].includedirs = [os.path.join("minion-bl", "include")]
            self.cpp_info.components["minion-bl"].libdirs = [os.path.join("minion-bl", "lib")]
            self.cpp_info.components["minion-bl"].libs = ["minion-bl"]
            self.cpp_info.components["minion-bl"].requires = ["etsoc_hal::etsoc_hal"]

        if self.options.with_mm_rt_svcs:
            self.cpp_info.components["mm-rt-svcs"].set_property("cmake_target_name", "et-common-libs::mm-rt-svcs")
            self.cpp_info.components["mm-rt-svcs"].includedirs = [os.path.join("mm-rt-svcs", "include")]
            self.cpp_info.components["mm-rt-svcs"].libdirs = [os.path.join("mm-rt-svcs", "lib")]
            self.cpp_info.components["mm-rt-svcs"].libs = ["mm-rt-svcs"]
            self.cpp_info.components["mm-rt-svcs"].defines = ["MM_RT=1"]
            self.cpp_info.components["mm-rt-svcs"].requires = ["etsoc_hal::etsoc_hal"]

        if self.options.with_cm_rt_svcs:
            self.cpp_info.components["cm-rt-svcs"].set_property("cmake_target_name", "et-common-libs::cm-rt-svcs")
            self.cpp_info.components["cm-rt-svcs"].includedirs = [os.path.join("cm-rt-svcs", "include")]
            self.cpp_info.components["cm-rt-svcs"].libdirs = [os.path.join("cm-rt-svcs", "lib")]
            self.cpp_info.components["cm-rt-svcs"].libs = ["cm-rt-svcs"]
            self.cpp_info.components["cm-rt-svcs"].requires = ["etsoc_hal::etsoc_hal"]
