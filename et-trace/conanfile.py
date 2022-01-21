from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conans import tools
import re
import os


class EsperantoTraceConan(ConanFile):
    name = "esperantoTrace"
    url = "https://gitlab.esperanto.ai/software/et-trace"
    license = "Esperanto Technologies"

    settings = "os", "compiler", "arch", "build_type"
    options = {
        "with_tests": [True, False]
    }
    default_options = {
        "with_tests": True
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/et-trace.git",
        "revision": "auto",
    }
    generator = "CMakeDeps"

    def set_version(self):
        content = tools.load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
        version = re.search(r"set\(PROJECT_VERSION (.*)\)", content).group(1)
        self.version = version.strip()
    
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ET_TRACE_TEST"] = self.options.with_tests
        tc.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.with_tests and not tools.cross_building(self.settings):
            cmake.test()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib"))
    
    def package_id(self):
        self.info.header_only()

    def package_info(self):
        et_trace_cmake_name = "esperantoTrace::et_trace"
        self.cpp_info.components["et_trace"].names["cmake_find_package"] = et_trace_cmake_name
        self.cpp_info.components["et_trace"].names["cmake_find_package_multi"] = et_trace_cmake_name
        self.cpp_info.components["et_trace"].set_property("cmake_target_name", et_trace_cmake_name)
        self.cpp_info.components["et_trace"].includedirs = ["include", "include/esperanto"]