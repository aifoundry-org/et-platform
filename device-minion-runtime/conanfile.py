from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import rmdir
import os
import textwrap

required_conan_version = ">=1.53.0"


class DeviceMinionRuntimeConan(ConanFile):
    name = "device-minion-runtime"
    url = "git@gitlab.com:esperantotech/software/device-minion-runtime.git"
    homepage = "https://gitlab.com/esperantotech/software/device-minion-runtime"
    description = "minion-rt runtime"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "warnings_as_errors": [True, False]
    }
    default_options = {
        "warnings_as_errors": True
    }

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, "deviceMinionRuntime")

    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)

    def layout(self):
        cmake_layout(self)

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def requirements(self):
        # header-only libs
        self.requires("deviceApi/2.1.0")
        self.requires("esperantoTrace/2.0.0")
        self.requires("signedImageFormat/1.3.0")
        # libs
        self.requires("etsoc_hal/1.6.0")
        self.requires("et-common-libs/0.20.0")

    def package_id(self):
        self.python_requires["conan-common"].module.x86_64_compatible(self)

    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.4.1 <1.0.0]")

    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def validate(self):
        if self.settings.arch != "rv64":
            raise ConanInvalidConfiguration("Cross-compiling to arch {} is not supported".format(self.settings.arch))

        et_common_libs = self.dependencies["et-common-libs"]
        # et-common-libs must be compiled with these components
        for flag in ["with_minion_bl", "with_mm_rt_svcs", "with_cm_rt_svcs"]:
            if not et_common_libs.options.get_safe(flag):
                raise ConanInvalidConfiguration("{0} requires {1} package with '-o {1}:{2}'".format(self.name, "et-common-libs", flag))

    def generate(self):
        make_hash_array = self.python_requires["conan-common"].module.make_hash_array
        make_version_array = self.python_requires["conan-common"].module.make_version_array

        tc = CMakeToolchain(self)
        tc.variables["GIT_HASH_STRING"] = self.info.package_id()
        tc.variables["GIT_HASH_ARRAY"] = make_hash_array(self.info.package_id())
        tc.variables["GIT_VERSION_STRING"] = self.version
        tc.variables["GIT_VERSION_ARRAY"] = make_version_array(self.version)

        tc.variables["ENABLE_STRICT_BUILD_TYPES"] = True
        tc.variables["ENABLE_WARNINGS_AS_ERRORS"] = self.options.warnings_as_errors
        tc.variables["DEVICE_MINION_RUNTIME_DEPRECATED"] = True
        tc.variables["BUILD_DOC"] = False
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder,
                                                         "cmake")
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"

        tc.preprocessor_definitions["RISCV_ET_MINION"] = ""

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
        elfs = [("MachineMinion.elf", "MachineMinion"),
                ("MasterMinion.elf", "MasterMinion"),
                ("MasterMinionTF.elf", "MasterMinion"),
                ("WorkerMinion.elf", "WorkerMinion")]
        return elfs

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

        build_modules_folder = os.path.join(self.package_folder, "lib", "cmake")
        os.makedirs(build_modules_folder)
        for elf, elf_dir in self._elfs:
            build_module_path = os.path.join(build_modules_folder, "conan-{}-{}.cmake".format(self.name, elf))
            with open(build_module_path, "w+") as f:
                f.write(textwrap.dedent("""\
                    if(NOT TARGET EsperantoDeviceMinionRuntime::{exec})
                        if(CMAKE_CROSSCOMPILING)
                            find_program(ESPERANTO_DEVICE_MINION_RUNTIME_{exec}_PROGRAM et-minion-runtime-{exec} PATHS ENV PATH NO_DEFAULT_PATH)
                        endif()
                        if(NOT ESPERANTO_DEVICE_MINION_RUNTIME_{exec}_PROGRAM)
                            set(ESPERANTO_DEVICE_MINION_RUNTIME_{exec}_PROGRAM "${{CMAKE_CURRENT_LIST_DIR}}/../../lib/esperanto-fw/{exec_dir}/{exec}")
                        endif()
                        get_filename_component(ESPERANTO_DEVICE_MINION_RUNTIME_{exec}_PROGRAM "${{ESPERANTO_DEVICE_MINION_RUNTIME_{exec}_PROGRAM}}" ABSOLUTE)
                        add_executable(EsperantoDeviceMinionRuntime::{exec} IMPORTED)
                        set_property(TARGET EsperantoDeviceMinionRuntime::{exec} PROPERTY IMPORTED_LOCATION ${{ESPERANTO_DEVICE_MINION_RUNTIME_{exec}_PROGRAM}})
                    endif()
                    """.format(exec=elf, exec_dir=elf_dir)))

        build_module_path = os.path.join(build_modules_folder, "conan-{}-{}.cmake".format(self.name, "deprecated-vars"))
        with open(build_module_path, "w+") as f:
            f.write(textwrap.dedent("""\
                set(ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../bin")
                get_filename_component(ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR "${{ESPERANTO_DEVICE_MINION_RUNTIME_BIN_DIR}}" ABSOLUTE)

                set(ESPERANTO_DEVICE_MINION_RUNTIME_INCLUDE_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../include")
                get_filename_component(ESPERANTO_DEVICE_MINION_RUNTIME_INCLUDE_DIR "${{ESPERANTO_DEVICE_MINION_RUNTIME_INCLUDE_DIR}}" ABSOLUTE)

                set(ESPERANTO_DEVICE_MINION_RUNTIME_LIB_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../lib")
                get_filename_component(ESPERANTO_DEVICE_MINION_RUNTIME_LIB_DIR "${{ESPERANTO_DEVICE_MINION_RUNTIME_LIB_DIR}}" ABSOLUTE)
                """.format(exec=elf, exec_dir=elf_dir)))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "EsperantoDeviceMinionRuntime")
        self.cpp_info.set_property("cmake_target_name", "EsperantoDeviceMinionRuntime")

        build_modules = []
        build_modules.append(os.path.join("lib", "cmake", "conan-{}-{}.cmake".format(self.name, "deprecated-vars")))
        for elf, elf_dir in self._elfs:
            build_modules.append(os.path.join("lib", "cmake", "conan-{}-{}.cmake".format(self.name, elf)))
        self.cpp_info.set_property("cmake_build_modules", build_modules)
