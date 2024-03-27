from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.layout import cmake_layout
from conan.tools.files import rmdir
import os


class DeviceManagementConan(ConanFile):
    name = "deviceManagement"
    url = "git@gitlab.com:esperantotech/software/devicemanagement.git"
    homepage = "https://gitlab.com/esperantotech/software/devicemanagement"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "fPIC": [True, False],
        "with_tests": [True, False],
    }
    default_options = {
        "fPIC": True,
        "with_tests": False,
    }

    generators = "CMakeDeps"

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)

    def requirements(self):
        self.requires("deviceApi/2.3.0-alpha")
        self.requires("deviceLayer/2.2.0")
        self.requires("hostUtils/0.3.0")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

        if self.options.with_tests:
            raise ConanInvalidConfiguration("Support for building with tests not yet implemented.")

    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)

    def layout(self):
        cmake_layout(self)
        self.folders.source = "."

    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.with_tests
        tc.variables["BUILD_DOC"] = False
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        # library components
        self.cpp_info.components["DM"].set_property("cmake_target_name", "deviceManagement::DM")
        self.cpp_info.components["DM"].requires = ["deviceApi::deviceApi", "deviceLayer::deviceLayer", "hostUtils::logging"]
        self.cpp_info.components["DM"].libs = ["DM"]
        self.cpp_info.components["DM"].includedirs = ["include"]
        self.cpp_info.components["DM"].libdirs = ["lib"]
        if self.settings.build_type != "Debug":
            self.cpp_info.components["DM"].defines.append("NDEBUG")

        self.cpp_info.components["DM_static"].set_property("cmake_target_name", "deviceManagement::DM_static")
        self.cpp_info.components["DM_static"].requires = ["deviceApi::deviceApi", "deviceLayer::deviceLayer", "hostUtils::logging"]
        self.cpp_info.components["DM_static"].libs = ["DM_static"]
        self.cpp_info.components["DM_static"].includedirs = ["include"]
        self.cpp_info.components["DM_static"].libdirs = ["lib"]
        if self.settings.build_type != "Debug":
            self.cpp_info.components["DM"].defines.append("NDEBUG")

        bin_path = os.path.join(self.package_folder, "bin")
        self.output.info("Appending PATH environment variable: {}".format(bin_path))
        self.env_info.PATH.append(bin_path)
