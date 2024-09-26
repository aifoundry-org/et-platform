from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.build import can_run
from conan.tools.files import copy, rmdir
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
        "with_tools": [True, False],
        "with_tests": [True, False],
        "disable_easy_profiler": [True, False],
        "run_tests": [True, False],
        "run_tests_sdk": ["v1.3.0", "latest"]  # TODO: once newer SDK(S) are released, add them here (with current + next should be enough)
    }
    default_options = {
        "with_tools": False,
        "with_tests": False,
        "disable_easy_profiler": False,
        "run_tests": False,
        "run_tests_sdk": "v1.3.0",
    }

    generators = "CMakeDeps"

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)

    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)
        # This conanfile_device_depends_sw_stack_v1.3.0.txt file is intended to be used by this conanfile.py recipe
        # to download device FW artifacts needed to build with tests.
        copy(self, "conanfile_device_depends_sw_stack_latest.txt", self.recipe_folder, self.export_folder)
        copy(self, "conanfile_device_depends_sw_stack_v1.3.0.txt", self.recipe_folder, self.export_folder)
    
    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def configure_options(self):
        if self.options.with_tests and not self.dependencies["esperanto-flash-tool"].options.get_safe("header_only"):
            raise ConanInvalidConfiguration("When enabling runtime tests esperanto-flash-tool:header_only must be True")

    def layout(self):
        cmake_layout(self)
        self.folders.source = "."
        et_runtime_test_kernels_dir = os.path.join("res", "esperanto-test-kernels", "lib", "esperanto-fw", "kernels")
        self.layouts.source.buildenv_info.define_path('ET_RUNTIME_TEST_KERNELS_DIR', et_runtime_test_kernels_dir)

    @property
    def _conanfile_device_artifacts(self):
        sdk_version = f"{self.options.run_tests_sdk}"
        return f"conanfile_device_depends_sw_stack_{sdk_version}.txt"

    def requirements(self):
        self.requires("deviceApi/2.1.0")
        self.requires("deviceLayer/3.0.0")
        self.requires("hostUtils/0.3.0")

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
            self.run(f"conan install {sysemu_artifacts_conanfile} -pr:b default -pr:h {host_ctx_profile} {extra_settings} --remote conan-develop --build missing -g deploy")

        if self.options.with_tools:
            self.requires("nlohmann_json/3.11.2")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        device_api = self.dependencies["deviceApi"]
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.get_safe("with_tests")
        tc.variables["BUILD_TOOLS"] = self.options.get_safe("with_tools")
        tc.variables["DISABLE_EASY_PROFILER"] = not self.options.get_safe("disable_easy_profiler")
        tc.variables["BUILD_DOCS"] = False
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

    def package_info(self):
        # library components
        self.cpp_info.components["etrt"].names["cmake_find_package"] = "etrt"
        self.cpp_info.components["etrt"].names["cmake_find_package_multi"] = "etrt"
        self.cpp_info.components["etrt"].set_property("cmake_target_name", "runtime::etrt")
        self.cpp_info.components["etrt"].requires = ["hostUtils::debug", "deviceApi::deviceApi", "libcap::libcap", "cereal::cereal", "deviceLayer::deviceLayer", "hostUtils::logging", "hostUtils::threadPool", "hostUtils::actionList", "elfio::elfio", "easy_profiler::easy_profiler"]
        self.cpp_info.components["etrt"].libs = ["etrt"]
        self.cpp_info.components["etrt"].includedirs = ["include"]
        self.cpp_info.components["etrt"].libdirs = ["lib"]

        self.cpp_info.components["etrt_static"].names["cmake_find_package"] = "etrt_static"
        self.cpp_info.components["etrt_static"].names["cmake_find_package_multi"] = "etrt_static"
        self.cpp_info.components["etrt_static"].set_property("cmake_target_name", "runtime::etrt_static")
        self.cpp_info.components["etrt_static"].requires = ["hostUtils::debug", "deviceApi::deviceApi", "libcap::libcap", "cereal::cereal", "deviceLayer::deviceLayer", "hostUtils::logging", "hostUtils::threadPool", "hostUtils::actionList", "elfio::elfio", "easy_profiler::easy_profiler"]
        self.cpp_info.components["etrt_static"].libs = ["etrt_static"]
        self.cpp_info.components["etrt_static"].includedirs = ["include"]
        self.cpp_info.components["etrt_static"].libdirs = ["lib"]
        
        binpath = os.path.join(self.package_folder, "bin")
        self.output.info("Appending PATH env var: {}".format(binpath))
        self.runenv_info.prepend_path('PATH', binpath)
        self.runenv_info.define_path('ET_RUNTIME_TEST_KERNELS_DIR', os.path.join(self.package_folder, "res", "esperanto-test-kernels", "lib", "esperanto-fw", "kernels"))

        # TODO: to remove in conan v2 once old virtualrunenv is removed
        self.env_info.PATH.append(binpath)
