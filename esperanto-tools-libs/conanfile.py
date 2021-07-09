from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
from conans.errors import ConanInvalidConfiguration
import os


class RuntimeConan(ConanFile):
    name = "runtime"
    version = "1.0"
    url = "https://gitlab.esperanto.ai/software/esperanto-tools-libs"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "fPIC": [True, False]
    }
    default_options = {
        "fPIC": True
    }

    generators = "cmake_find_package_multi"

    exports_sources = [ "CMakeLists.txt", "cmake/*", "include/*", "src/*", "runtimeConfig.cmake.in" ]

    requires = [ 
        "deviceApi/0.1.0",
        "deviceLayer/1.0",
        "hostUtils/1.0",
        "cereal/1.3.0",
        "elfio/3.8"
    ]
    build_requires = "cmake-modules/[>=0.4.1 <1.0.0]"
    python_requires = "conan-common/[>=0.1.0 <1.0.0]"

    @property
    def _build_subfolder(self):
        return "build_subfolder"
    
    def configure(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = False
        tc.variables["BUILD_DOCS"] = False
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.deps_cpp_info["cmake-modules"].rootpath, "cmake")
        tc.generate()
    

    _cmake = None
    def _configure_cmake(self):
        if not self._cmake:
            cmake = CMake(self, build_folder=self._build_subfolder)
            cmake.verbose = True
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
        self.cpp_info.components["etrt"].names["cmake_find_package"] = "etrt"
        self.cpp_info.components["etrt"].names["cmake_find_package_multi"] = "etrt"
        self.cpp_info.components["etrt"].requires = ["hostUtils::debug", "cereal::cereal", "deviceApi::deviceApi", "deviceLayer::deviceLayer", "hostUtils::logging", "elfio::elfio"]
        self.cpp_info.components["etrt_static"].names["cmake_find_package"] = "etrt_static"
        self.cpp_info.components["etrt_static"].names["cmake_find_package_multi"] = "etrt_static"
        self.cpp_info.components["etrt_static"].requires = ["hostUtils::debug", "cereal::cereal", "deviceApi::deviceApi", "deviceLayer::deviceLayer", "hostUtils::logging", "elfio::elfio"]