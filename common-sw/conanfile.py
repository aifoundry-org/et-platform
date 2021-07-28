from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
import os

class HostUtilsConan(ConanFile):
    name = "hostUtils"
    version = "0.1.0"
    url = "https://gitlab.esperanto.ai/software/common-sw"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True
    }

    generators = "cmake_find_package_multi"

    exports_sources = [ "CMakeLists.txt", "logging/*", "debug/*", "hostUtilsConfig.cmake.in" ]

    requires = "g3log/1.3.2"
    build_requires = "cmake-modules/[>=0.4.1 <1.0.0]"
    python_requires = "conan-common/[>=0.1.0 <1.0.0]"
    
    def configure(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.deps_cpp_info["cmake-modules"].rootpath, "cmake")
        tc.generate()
    
    _cmake = None
    def _configure_cmake(self):
        if not self._cmake:
            cmake = CMake(self)
            cmake.configure()
            self._cmake = cmake
        return self._cmake
    
    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
    
    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
    
    def package_info(self):
        # library components
        self.cpp_info.components["logging"].set_property("cmake_target_name", "logging")
        self.cpp_info.components["logging"].requires = ["g3log::g3log"]
        self.cpp_info.components["logging"].libs = ["logging"]
        self.cpp_info.components["logging"].libdirs = ["lib", "lib64"]
        self.cpp_info.components["debug"].set_property("cmake_target_name", "debug")
        self.cpp_info.components["debug"].requires = ["g3log::g3log"]
        self.cpp_info.components["debug"].libs = ["debugging"]
        self.cpp_info.components["debug"].libdirs = ["lib", "lib64"]
