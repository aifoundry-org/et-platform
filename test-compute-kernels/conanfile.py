from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import rmdir
import os
import textwrap

required_conan_version = ">=1.53.0"


class EsperantoTestKenelsConan(ConanFile):
    name = "esperanto-test-kernels"
    url = "git@gitlab.com:esperantotech/software/test-compute-kernels.git"
    homepage = "https://gitlab.com/esperantotech/software/test-compute-kernels"
    description = "Suite of test kernels"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {}
    default_options = {}

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, "EsperantoTestKernels")

    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)

    def configure(self):
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def requirements(self):
        self.requires("esperantoTrace/[>=1.0.0 <2.0.0]")
        self.requires("et-common-libs/[>=0.10.0 <1.0.0]")

    def package_id(self):
        self.python_requires["conan-common"].module.x86_64_compatible(self)

    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.4.1 <1.0.0]")

    def validate(self):
        if self.settings.arch != "rv64":
            raise ConanInvalidConfiguration("Cross-compiling to arch {} is not supported".format(self.settings.arch))

        et_common_libs = self.dependencies["et-common-libs"]
        # et-common-libs must be compiled with these components
        for flag in ["with_cm_umode"]:
            if not et_common_libs.options.get_safe(flag):
                raise ConanInvalidConfiguration("{0} requires {1} package with '-o {1}:{2}'".format(self.name, "et-common-libs", flag))

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
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
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

        build_modules_folder = os.path.join(self.package_folder, "lib", "cmake")
        os.makedirs(build_modules_folder)
        build_module_path = os.path.join(build_modules_folder, "conan-{}-{}.cmake".format(self.name, "deprecated-vars"))
        with open(build_module_path, "w+") as f:
            f.write(textwrap.dedent("""\
                set(ESPERANTO_TEST_KERNELS_BIN_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../bin")
                get_filename_component(ESPERANTO_TEST_KERNELS_BIN_DIR "${{ESPERANTO_TEST_KERNELS_BIN_DIR}}" ABSOLUTE)

                set(ESPERANTO_TEST_KERNELS_INCLUDE_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../include")
                get_filename_component(ESPERANTO_TEST_KERNELS_INCLUDE_DIR "${{ESPERANTO_TEST_KERNELS_INCLUDE_DIR}}" ABSOLUTE)

                set(ESPERANTO_TEST_KERNELS_LIB_DIR "${{CMAKE_CURRENT_LIST_DIR}}/../../lib")
                get_filename_component(ESPERANTO_TEST_KERNELS_LIB_DIR "${{ESPERANTO_TEST_KERNELS_LIB_DIR}}" ABSOLUTE)
                """.format()))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "EsperantoTestKernels")

        build_modules = []
        build_modules.append(os.path.join("lib", "cmake", "conan-{}-{}.cmake".format(self.name, "deprecated-vars")))
        self.cpp_info.set_property("cmake_build_modules", build_modules)
