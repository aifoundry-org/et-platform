from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.layout import cmake_layout
from conan.tools.files import rmdir
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
        "with_tests": False
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/et-trace.git",
        "revision": "auto",
    }
    generator = "CMakeDeps"

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)
    
    def layout(self):
        cmake_layout(self)
    
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ET_TRACE_TEST"] = self.options.with_tests
        tc.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.with_tests and not can_run(self):
            cmake.test()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib"))
    
    def package_id(self):
        self.info.clear()
        
        # If user requests package without tests but only packages with tests are available
        # it is legal to use a package with tests
        if not self.options.with_tests:
            compatible_pkg = self.info.clone()
            compatible_pkg.options.with_tests = True
            self.compatible_packages.append(compatible_pkg)

    def package_info(self):
        self.cpp_info.components["et_trace"].set_property("cmake_target_name",  "esperantoTrace::et_trace")
        self.cpp_info.components["et_trace"].includedirs = ["include", "include/esperanto"]

        # TODO: to remove in conan v2 once cmake_find_package* generators removed
        # changes namespace to esperantoTrace::
        self.cpp_info.names["cmake_find_package"] = "esperantoTrace" 
        # yields 'esperantoTrace::et_trace' target
        self.cpp_info.components["et_trace"].names["cmake_find_package"] = "et_trace"
        self.cpp_info.components["et_trace"].names["cmake_find_package_multi"] = "et_trace"
