from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import can_run, cross_building
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import copy, rmdir
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.layout import cmake_layout
import os


class RuntimeConan(ConanFile):
    name = "runtime"
    url = "git@gitlab.com:esperantotech/software/esperanto-tools-libs.git"
    homepage = "https://gitlab.com/esperantotech/software/esperanto-tools-libs"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "fvisibility": ["default", "protected", "hidden"],
        "with_tools": [True, False],
        "with_tests": [True, False],
        "disable_easy_profiler": [True, False],
        "run_tests": [True, False],
        "run_tests_sdk": ["v1.3.3", "v1.4.4", "v1.5.3", "v1.6.2", "latest"]  # TODO: once newer SDK(S) are released, add them here (with current + next should be enough)
    }
    default_options = {
        "fvisibility": "default",
        "with_tools": False,
        "with_tests": False,
        "disable_easy_profiler": False,
        "run_tests": False,
        "run_tests_sdk": "v1.6.2",
    }

    generators = "CMakeDeps"

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)

    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)
        # This conanfile_device_depends_sw_stack_v1.3.3.txt file is intended to be used by this conanfile.py recipe
        # to download device FW artifacts needed to build with tests.
        copy(self, "conanfile_device_depends_sw_stack_latest.txt", self.recipe_folder, self.export_folder)
        copy(self, "conanfile_device_depends_sw_stack_v1.6.2.txt", self.recipe_folder, self.export_folder)
        copy(self, "conanfile_device_depends_sw_stack_v1.5.3.txt", self.recipe_folder, self.export_folder)
        copy(self, "conanfile_device_depends_sw_stack_v1.4.4.txt", self.recipe_folder, self.export_folder)
        copy(self, "conanfile_device_depends_sw_stack_v1.3.3.txt", self.recipe_folder, self.export_folder)
    
    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def configure_options(self):
        if self.options.with_tests and not self.dependencies["esperanto-flash-tool"].options.get_safe("header_only"):
            raise ConanInvalidConfiguration("When enabling runtime tests esperanto-flash-tool:header_only must be True")

    @property
    def _etrt_components(self):
        common_requires = ["hostUtils::debug", "deviceApi::deviceApi", "libcap::libcap", "cereal::cereal", "deviceLayer::deviceLayer", "hostUtils::logging", "hostUtils::threadPool", "hostUtils::actionList", "elfio::elfio", "easy_profiler::easy_profiler"]
        return {
            "etrt": {
                "cmake_target": "runtime::etrt",
                "libs": ["etrt"],
                "requires": common_requires,
                "includedirs": {
                    "source": ["include"],
                    "build": [],
                    "package": ["include"],
                },
                "libdirs": {
                    "source": [],
                    "build": ["."],
                    "package": ["lib"],
                },
            },
            "etrt_static": {
                "cmake_target": "runtime::etrt_static",
                "libs": ["etrt_static"],
                "requires": common_requires,
                "includedirs": {
                    "source": ["include"],
                    "build": [],
                    "package": ["include"],
                },
                "libdirs": {
                    "source": [],
                    "build": ["."],
                    "package": ["lib"],
                },
            }
        }

    @property
    def _etrt_envvars(self):
        et_runtime_test_kernels_dir = os.path.join("res", "esperanto-test-kernels", "lib", "esperanto-fw", "kernels")
        return {
            "ET_RUNTIME_TEST_KERNELS_DIR": {
                "action": "define_path",
                "source": et_runtime_test_kernels_dir,
                "package": et_runtime_test_kernels_dir,
            },
            "PATH": {
                "action": "prepend_path",
                "source": "scripts",
                "build": self.cpp.build.bindirs[0],
                "package": self.cpp.package.bindirs[0],
            }
        }

    def layout(self):
        cmake_layout(self)

        for cpp_info in [self.cpp.build, self.cpp.package]:
            for component, values in self._etrt_components.items():
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
            for component, values in self._etrt_components.items():
                cpp_info.components[component].includedirs = values.get("includedirs", dict()).get(name, [])
                cpp_info.components[component].libdirs = values.get("libdirs", dict()).get(name, [])
                cpp_info.components[component].bindirs = values.get("bindirs", dict()).get(name, [])

        for layout_info, name in [(self.layouts.source, "source"), (self.layouts.build, "build"), (self.layouts.package, "package")]:
            for envvar, values in self._etrt_envvars.items():
                envvar_action = values.get("action")
                envvar_path = values.get(name)
                if envvar_path:
                    if isinstance(envvar_path, list):
                        for envvar_path_item in envvar_path:
                            getattr(layout_info.runenv_info, envvar_action)(envvar, envvar_path_item)
                    else:
                        getattr(layout_info.runenv_info, envvar_action)(envvar, envvar_path)

    @property
    def _conanfile_device_artifacts(self):
        sdk_version = f"{self.options.run_tests_sdk}"
        return f"conanfile_device_depends_sw_stack_{sdk_version}.txt"

    def requirements(self):
        self.requires("deviceApi/2.1.0")
        self.requires("deviceLayer/4.0.0-alpha")
        self.requires("et-host-utils/0.4.0-alpha")

        self.requires("cereal/1.3.2")
        self.requires("elfio/3.8")
        self.requires("libcap/2.62")
        self.requires("gflags/2.2.2")

        self.requires("easy_profiler/2.1.0")            #need this nevertheless for the include files

        self.requires("cmake-modules/[>=0.4.1 <1.0.0]")
        
        if self.options.with_tests:
            self.requires("gtest/1.10.0")
            self.requires("sw-sysemu/[>=0.5.0 <1.0.0]")

            sysemu_artifacts_conanfile = os.path.join(self.recipe_folder, self._conanfile_device_artifacts)
            host_ctx_profile = "baremetal-rv64-gcc8.2-debug" if self.settings.build_type == "Debug" else "baremetal-rv64-gcc8.2-release"
            extra_settings = ""
            if self.settings.build_type == "Debug":
                extra_settings = "-s:h *:build_type=Release"
            self.run(f"conan install {sysemu_artifacts_conanfile} -pr:b default -pr:h {host_ctx_profile} {extra_settings} -if=res --remote conan-develop --build missing -g deploy")

        if self.options.with_tools:
            self.requires("nlohmann_json/3.11.2")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        vbe = VirtualBuildEnv(self)
        vbe.generate()
        if not cross_building(self):
            vre = VirtualRunEnv(self)
            vre.generate(scope="build")

        device_api = self.dependencies["deviceApi"]
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.get_safe("with_tests")
        tc.variables["BUILD_TOOLS"] = self.options.get_safe("with_tools")
        tc.variables["DISABLE_EASY_PROFILER"] = not self.options.get_safe("disable_easy_profiler")
        tc.variables["BUILD_DOCS"] = False
        tc.variables["CMAKE_ASM_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_C_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_CXX_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_VISIBILITY_INLINES_HIDDEN"] = "ON" if self.options.fvisibility == "hidden" else "OFF"
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.deps_cpp_info["cmake-modules"].rootpath, "cmake")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.with_tests and can_run(self) and self.options.run_tests:
            self.run("sudo ctest -L 'Generic' --no-compress-output")

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        copy(self, "*", src=os.path.join(self.source_folder, "esperanto-test-kernels"), dst=os.path.join(self.package_folder, "res", "esperanto-test-kernels"))
