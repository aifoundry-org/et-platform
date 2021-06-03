from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
from conans.errors import ConanInvalidConfiguration

class HostUtilsConan(ConanFile):
    name = "hostUtils"
    version = "1.0"
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
    python_requires = "conan-common/[>=0.1.0 <1.0.0]"

    @property
    def _build_subfolder(self):
        return "build_subfolder"
    
    def configure(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def generate(self):
        tc = CMakeToolchain(self)
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
        self.cpp_info.components["logging"].names["cmake_find_package"] = "logging"
        self.cpp_info.components["logging"].names["cmake_find_package_multi"] = "logging"
        self.cpp_info.components["logging"].requires = ["g3log::g3log"]