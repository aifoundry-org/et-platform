from conan import ConanFile
from conan.tools.build import can_run, cross_building
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
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
        "fPIC": [True, False],
        "with_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
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

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def config_options(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("g3log/1.3.3", transitive_headers=True, transitive_libs=True)

    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.4.1 <1.0.0]")
        self.tool_requires("cmake/[>=3.21 <4]")
        if self.options.with_tests:
            self.test_requires("gtest/1.10.0")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    @property
    def _components(self):
        common_requires = ["g3log::g3log"]
        def component_template(name):
            return {
                    "cmake_target": f"hostUtils::{name}",
                    "libs": [f"{name}"],
                    "requires": common_requires,
                    "includedirs": {
                        "source": [f"src/{name}/include"],
                        "build": [],
                        "package": ["include"],
                    },
                    "libdirs": {
                        "source": [],
                        "build": [f"src/{name}"],
                        "package": ["lib", "lib64"],
                    }
                }

        components = {
            "logging": component_template("logging"),
            "debug": component_template("debug"),
            "debugging": {
                "cmake_target": f"hostUtils::debugging",
                "libs": [f"debugging"],
                "requires": common_requires,
                "includedirs": {
                    "source": [f"src/debug/include"],
                    "build": [],
                    "package": ["include"],
                },
                "libdirs": {
                    "source": [],
                    "build": [f"src/debug"],
                    "package": ["lib", "lib64"],
                }
            },
            "threadPool": component_template("threadPool"),
            "actionList": component_template("actionList"),
        }
        return components

    def layout(self):
        cmake_layout(self)

        for cpp_info in [self.cpp.build, self.cpp.package]:
            for component, values in self._components.items():
                cmake_target = values["cmake_target"]

                libs = values.get("libs", [])
                defines = values.get("defines", [])
                cxxflags = values.get("cxxflags", [])
                linkflags = values.get("linkflags", [])
                system_libs = []
                for _condition, _system_libs in values.get("system_libs", []):
                    if _condition:
                        system_libs.extend(_system_libs)
                frameworks = values.get("frameworks", [])
                requires = values.get("requires", [])

                cpp_info.components[component].set_property("cmake_target_name", cmake_target)
                cpp_info.components[component].set_property("pkg_config_name", component)

                cpp_info.components[component].libs = libs
                cpp_info.components[component].defines = defines
                cpp_info.components[component].cxxflags = cxxflags
                cpp_info.components[component].sharedlinkflags = linkflags
                cpp_info.components[component].exelinkflags = linkflags
                cpp_info.components[component].system_libs = system_libs
                cpp_info.components[component].frameworks = frameworks
                cpp_info.components[component].requires = requires

        for cpp_info, name in [(self.cpp.source, "source"), (self.cpp.build, "build"), (self.cpp.package, "package")]:
            for component, values in self._components.items():
                cpp_info.components[component].includedirs = values.get("includedirs", dict()).get(name, [])
                cpp_info.components[component].libdirs = values.get("libdirs", dict()).get(name, [])
                cpp_info.components[component].bindirs = values.get("bindirs", dict()).get(name, [])

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        vbe = VirtualBuildEnv(self)
        vbe.generate()
        if not cross_building:
            vre = VirtualRunEnv(self)
            vre.generate(scope="build")

        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.with_tests
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

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
        self.cpp_info.set_property("cmake_file_name", "hostUtils")
        self.cpp_info.set_property("cmake_target_name", "hostUtils::hostUtils")
