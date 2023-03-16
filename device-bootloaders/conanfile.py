from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import rmdir
import textwrap
import os


required_conan_version = ">=1.53.0"


class EsperantoBootLoadersConan(ConanFile):
    name = "device-bootloaders"
    url = "https://gitlab.esperanto.ai/software/device-bootloaders"
    description = "Device Bootloaders"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "warnings_as_errors" : [True, False]
    }
    default_options = {
        "warnings_as_errors" : False
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/device-bootloaders.git",
        "revision": "auto",
    }

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, "EsperantoBootLoader")

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def requirements(self):
        # header-only libs
        self.requires("deviceApi/[>=1.1.0 <2.0.0]")
        self.requires("esperantoTrace/[>=1.1.0 <2.0.0]")
        self.requires("signedImageFormat/1.2.0")
        self.requires("tf-protocol/[>=1.0.0 <2.0.0]")
        self.requires("esperanto-flash-tool/[>=1.0.0 <2.0.0]") # we only consume a header
        # libs
        self.requires("etsoc_hal/[>=1.4.0 <2.0.0]")
        self.requires("et-common-libs/[>=0.14.0 <1.0.0]")

    def package_id(self):
        self.python_requires["conan-common"].module.x86_64_compatible(self)

    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.4.1 <1.0.0]")

    def validate(self):
        if self.settings.arch != "rv64":
            raise ConanInvalidConfiguration("Cross-compiling to arch %s is not supported" % self.settings.arch)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        make_hash_array = self.python_requires["conan-common"].module.make_hash_array
        make_version_array = self.python_requires["conan-common"].module.make_version_array

        tc = CMakeToolchain(self)
        tc.variables["CMAKE_VERBOSE_MAKEFILE"] = False
        tc.variables["GIT_HASH_STRING"] = self.info.package_id()
        tc.variables["GIT_HASH_ARRAY"] = make_hash_array(self.info.package_id())
        tc.variables["GIT_VERSION_STRING"] = self.version
        tc.variables["GIT_VERSION_ARRAY"] = make_version_array(self.version)

        tc.variables["ENABLE_WARNINGS_AS_ERRORS"] = self.options.warnings_as_errors
        tc.variables["BUILD_DOC"] = False
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"

        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    @property
    def _elfs(self):
        #            (executable, directory name)
        elfs = [
            ("BootromTrampolineToBL2.elf", "BootromTrampolineToBL2"),
            ("ServiceProcessorBL1.elf", "ServiceProcessorBL1"),
            ("ServiceProcessorBL2_production.elf", os.path.join("ServiceProcessorBL2", "production")),
            ("ServiceProcessorBL2_fast-boot.elf", os.path.join("ServiceProcessorBL2", "fast-boot")),
            ("ServiceProcessorBL2_testframework.elf", os.path.join("ServiceProcessorBL2", "testframework")),
            ("ServiceProcessorBL2_mdi_enabled.elf", os.path.join("ServiceProcessorBL2", "mdi_enabled"))
        ]
        return elfs

    def _custom_cmake_module(self, name):
        return "conan-{}-{}.cmake".format(self.name, name)

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

        build_modules_folder = os.path.join(self.package_folder, "lib", "cmake")
        os.makedirs(build_modules_folder)
        for elf, elf_dir in self._elfs:
            build_module_path = os.path.join(build_modules_folder, self._custom_cmake_module(elf))
            with open(build_module_path, "w+") as f:
                f.write(textwrap.dedent("""\
                    if(NOT TARGET EsperantoBootLoader::{exec})
                        if(CMAKE_CROSSCOMPILING)
                            find_program(ESPERANTO_BOOTLOADER_{exec}_PROGRAM et-bootloader-{exec} PATHS ENV PATH NO_DEFAULT_PATH)
                        endif()
                        if(NOT ESPERANTO_BOOTLOADER_{exec}_PROGRAM)
                            set(ESPERANTO_BOOTLOADER_{exec}_PROGRAM "${{CMAKE_CURRENT_LIST_DIR}}/../../lib/esperanto-fw/{exec_dir}/{exec}")
                        endif()
                        get_filename_component(ESPERANTO_BOOTLOADER_{exec}_PROGRAM "${{ESPERANTO_BOOTLOADER_{exec}_PROGRAM}}" ABSOLUTE)
                        add_executable(EsperantoBootLoader::{exec} IMPORTED)
                        set_property(TARGET EsperantoBootLoader::{exec} PROPERTY IMPORTED_LOCATION ${{ESPERANTO_BOOTLOADER_{exec}_PROGRAM}})
                    endif()
                    """.format(exec=elf, exec_dir=elf_dir)))

        build_module_path = os.path.join(build_modules_folder, self._custom_cmake_module("deprecated-vars"))
        with open(build_module_path, "w+") as f:
            f.write(textwrap.dedent("""\
                set(ESPERANTO_BOOTLOADER_BIN_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../bin")
                get_filename_component(ESPERANTO_BOOTLOADER_BIN_DIR "${{ESPERANTO_BOOTLOADER_BIN_DIR}}" ABSOLUTE)

                set(ESPERANTO_BOOTLOADER_INCLUDE_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../include")
                get_filename_component(ESPERANTO_BOOTLOADER_INCLUDE_DIR "${{ESPERANTO_BOOTLOADER_INCLUDE_DIR}}" ABSOLUTE)

                set(ESPERANTO_BOOTLOADER_LIB_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../lib")
                get_filename_component(ESPERANTO_BOOTLOADER_LIB_DIR "${{ESPERANTO_BOOTLOADER_LIB_DIR}}" ABSOLUTE)
                """.format()))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "EsperantoBootLoader")

        build_modules = []
        build_modules.append(os.path.join("lib", "cmake", self._custom_cmake_module("deprecated-vars")))
        for elf, elf_dir in self._elfs:
            build_modules.append(os.path.join("lib", "cmake", self._custom_cmake_module(elf)))
        self.cpp_info.set_property("cmake_build_modules", build_modules)
