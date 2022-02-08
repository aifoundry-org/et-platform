from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conans import tools
from conans.errors import ConanInvalidConfiguration
import textwrap
import os
import re

class EtCommonLibsConan(ConanFile):
    name = "et-common-libs"
    url = "https://gitlab.esperanto.ai/software/et-common-libs.git"
    description = "Esperanto common libraries - used across SP, MM MMode, MM SMode, CM MMode, CM SMode, CM UMode."
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "with_sp_bl": [True, False],
        "with_cm_umode": [True, False],
        "with_minion_bl": [True, False],
        "with_mm_rt_svcs": [True, False],
        "with_cm_rt_svcs": [True, False],
    }
    default_options = {
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

    def set_version(self):
        content = tools.load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
        version = re.search(r"set\(PROJECT_VERSION (.*)\)", content).group(1)
        self.version = version.strip()
    
    def requirements(self):
        self.requires("esperantoTrace/0.1.0")
        # cm-umode doens't require etsoc_hal
        if self.options.with_sp_bl or \
           self.options.with_minion_bl or \
           self.options.with_mm_rt_svcs or \
           self.options.with_cm_rt_svcs:
           self.requires("etsoc_hal/0.1.0")

    def validate(self):
        if self.settings.arch != "rv64":
            raise ConanInvalidConfiguration("Cross-compiling to arch %s is not supported" % self.settings.arch)

    def package_id(self):
        # the target device doesn't have an OS
        del self.info.settings.os

    def generate(self):
        new_cmake_flags_init_template = textwrap.dedent("""
        set(CMAKE_CXX_FLAGS_INIT "${CONAN_CXX_FLAGS}" CACHE STRING "" FORCE)
        set(CMAKE_C_FLAGS_INIT "${CONAN_C_FLAGS} -std=gnu11 --specs=nano.specs -mcmodel=medany -march=rv64imf -mabi=lp64f -fno-zero-initialized-in-bss -ffunction-sections -fdata-sections -fstack-usage -Wall -Wextra -Werror -Wdouble-promotion -Wformat -Wnull-dereference -Wswitch-enum -Wshadow -Wstack-protector -Wpointer-arith -Wundef -Wbad-function-cast -Wcast-qual -Wcast-align -Wconversion -Wlogical-op -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wno-main" CACHE STRING "" FORCE)
        set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CONAN_SHARED_LINKER_FLAGS}" CACHE STRING "" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_INIT "${CONAN_EXE_LINKER_FLAGS}" CACHE STRING "" FORCE)
        """)

        tc = CMakeToolchain(self)
        tc.variables["WITH_SP_BL"] = self.options.with_sp_bl
        tc.variables["WITH_CM_UMODE"] = self.options.with_cm_umode
        tc.variables["WITH_MINION_BL"] = self.options.with_minion_bl
        tc.variables["WITH_MM_RT_SVCS"] = self.options.with_mm_rt_svcs
        tc.variables["WITH_CM_RT_SVCS"] = self.options.with_cm_rt_svcs
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"

        tc.blocks["cmake_flags_init"].template = new_cmake_flags_init_template

        if tools.cross_building(self):
            tc.blocks["generic_system"].values["compiler"] = self.deps_env_info["riscv-gnu-toolchain"].CC

        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
    
    def package_info(self):
        self.cpp_info.components["cm-umode"].includedirs = [os.path.join("cm-umode", "include")]
        self.cpp_info.components["cm-umode"].libdirs = [os.path.join("cm-umode", "lib")]
        self.cpp_info.components["cm-umode"].libs = ["cm-umode"]
        self.cpp_info.components["cm-umode"].requires = ["esperantoTrace::et_trace"]

        if self.options.with_sp_bl:
            self.cpp_info.components["sp_bl1"].includedirs = [os.path.join("sp-bl1", "include")]
            self.cpp_info.components["sp_bl1"].libdirs = [os.path.join("sp-bl1", "lib")]
            self.cpp_info.components["sp_bl1"].libs = ["sp-bl1"]
            self.cpp_info.components["sp_bl1"].requires = ["etsoc_hal::etsoc_hal"]

            self.cpp_info.components["sp_bl2"].includedirs = [os.path.join("sp-bl2", "include")]
            self.cpp_info.components["sp_bl2"].libdirs = [os.path.join("sp-bl2", "lib")]
            self.cpp_info.components["sp_bl2"].libs = ["sp-bl2"]
            self.cpp_info.components["sp_bl2"].defines = ["SERVICE_PROCESSOR_BL2=1"]
            self.cpp_info.components["sp_bl2"].requires = ["etsoc_hal::etsoc_hal", "esperantoTrace::et_trace"]

        if self.options.with_minion_bl:
            self.cpp_info.components["minion-bl"].includedirs = [os.path.join("minion-bl", "include")]
            self.cpp_info.components["minion-bl"].libdirs = [os.path.join("minion-bl", "lib")]
            self.cpp_info.components["minion-bl"].libs = ["minion-bl"]
            self.cpp_info.components["minion-bl"].requires = ["etsoc_hal::etsoc_hal"]

        if self.options.with_mm_rt_svcs:
            self.cpp_info.components["mm-rt-svcs"].includedirs = [os.path.join("mm-rt-svcs", "include")]
            self.cpp_info.components["mm-rt-svcs"].libdirs = [os.path.join("mm-rt-svcs", "lib")]
            self.cpp_info.components["mm-rt-svcs"].libs = ["mm-rt-svcs"]
            self.cpp_info.components["mm-rt-svcs"].defines = ["MM_RT=1"]
            self.cpp_info.components["mm-rt-svcs"].requires = ["etsoc_hal::etsoc_hal"]

        if self.options.with_cm_rt_svcs:
            self.cpp_info.components["cm-rt-svcs"].includedirs = [os.path.join("cm-rt-svcs", "include")]
            self.cpp_info.components["cm-rt-svcs"].libdirs = [os.path.join("cm-rt-svcs", "lib")]
            self.cpp_info.components["cm-rt-svcs"].libs = ["cm-rt-svcs"]
            self.cpp_info.components["cm-rt-svcs"].requires = ["etsoc_hal::etsoc_hal"]
