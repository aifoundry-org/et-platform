from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain
from conans.errors import ConanInvalidConfiguration
import os


class DeviceManagementConan(ConanFile):
    name = "deviceManagement"
    version = "0.1.0"
    url = "https://gitlab.esperanto.ai/software/deviceManagement" #TODO: Once deviceManagement gets its own project, update this url
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

    exports_sources = [ "CMakeLists.txt", "cmake/*", "include/*", "src/*", "deviceManagementConfig.cmake.in" ]

    requires = [
        # core lib
        "deviceApi/0.1.0",
        "deviceLayer/0.1.0",
        "hostUtils/0.1.0"
    ]
    build_requires = "cmake-modules/[>=0.4.1 <1.0.0]"
    python_requires = "conan-common/[>=0.1.0 <1.0.0]"
    
    def configure(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = False
        tc.variables["BUILD_TOOLS"] = False
        tc.variables["BUILD_DOC"] = False
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.deps_cpp_info["cmake-modules"].rootpath, "cmake")
        tc.generate()
    

    _cmake = None
    def _configure_cmake(self):
        if not self._cmake:
            cmake = CMake(self)
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
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        # library components
        self.cpp_info.components["DM"].names["cmake_find_package"] = "DM"
        self.cpp_info.components["DM"].names["cmake_find_package_multi"] = "DM"
        self.cpp_info.components["DM"].requires = ["deviceApi::deviceApi", "deviceLayer::deviceLayer", "hostUtils::logging"]
        self.cpp_info.components["DM"].lib = ["DM"]
        self.cpp_info.components["DM"].libdirs = ["lib", "lib64"]
        self.cpp_info.components["DM_static"].names["cmake_find_package"] = "DM_static"
        self.cpp_info.components["DM_static"].names["cmake_find_package_multi"] = "DM_static"
        self.cpp_info.components["DM_static"].requires = ["deviceApi::deviceApi", "deviceLayer::deviceLayer", "hostUtils::logging"]
        self.cpp_info.components["DM_static"].lib = ["DM_static"]
        self.cpp_info.components["DM_static"].libdirs = ["lib", "lib64"]
