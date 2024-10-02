from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.build import can_run
from conan.tools.env import VirtualRunEnv
from conan.tools.files import rmdir
import os


class HostUtilsConan(ConanFile):
    name = "et-host-utils"
    url = "git@gitlab.com:esperantotech/software/common-sw.git"
    homepage = "https://gitlab.com/esperantotech/software/common-sw"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "with_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "with_tests": False
    }

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, "hostUtils")

    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)

    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def layout(self):
        cmake_layout(self)
        self.cpp.source.includedirs = ["."]

    def requirements(self):
        self.requires("g3log/1.3.3")
        if self.options.with_tests:
            self.requires("gtest/1.10.0")
        
    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")
    
    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.4.1 <1.0.0]")

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.with_tests
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()
        vrenv = VirtualRunEnv(self)
        vrenv.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.with_tests and can_run(self):
            self.run("ctest", cwd=os.path.join("threadPool", "tests"), env="conanrun")
        if self.options.with_tests and can_run(self):
            self.run("ctest", cwd=os.path.join("actionList", "tests"), env="conanrun")            
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        # library components
        self.cpp_info.components["logging"].set_property("cmake_target_name", "hostUtils::logging")
        self.cpp_info.components["logging"].requires = ["g3log::g3log"]
        self.cpp_info.components["logging"].libs = ["logging"]
        self.cpp_info.components["logging"].includedirs =  ["include"]
        self.cpp_info.components["logging"].libdirs = ["lib", "lib64"]

        self.cpp_info.components["debug"].set_property("cmake_target_name", "hostUtils::debug")
        self.cpp_info.components["debug"].requires = ["g3log::g3log"]
        self.cpp_info.components["debug"].libs = ["debugging"]
        self.cpp_info.components["debug"].includedirs =  ["include"]
        self.cpp_info.components["debug"].libdirs = ["lib", "lib64"]

        self.cpp_info.components["debugging"].set_property("cmake_target_name", "hostUtils::debugging")
        self.cpp_info.components["debugging"].requires = ["g3log::g3log"]
        self.cpp_info.components["debugging"].libs = ["debugging"]
        self.cpp_info.components["debugging"].includedirs =  ["include"]
        self.cpp_info.components["debugging"].libdirs = ["lib", "lib64"]

        self.cpp_info.components["threadPool"].set_property("cmake_target_name", "hostUtils::threadPool")
        self.cpp_info.components["threadPool"].requires = ["logging"]
        if self.options.with_tests:
            self.cpp_info.components["threadPool"].requires.append("gtest::gmock")
        self.cpp_info.components["threadPool"].libs = ["threadPool"]
        self.cpp_info.components["threadPool"].includedirs =  ["include"]
        self.cpp_info.components["threadPool"].libdirs = ["lib", "lib64"]

        self.cpp_info.components["actionList"].set_property("cmake_target_name", "hostUtils::actionList")
        self.cpp_info.components["actionList"].requires = ["logging"]
        if self.options.with_tests:
            self.cpp_info.components["actionList"].requires.append("gtest::gmock")
        self.cpp_info.components["actionList"].libs = ["actionList"]
        self.cpp_info.components["actionList"].includedirs =  ["include"]
        self.cpp_info.components["actionList"].libdirs = ["lib", "lib64"]
