from conan import ConanFile
from conan.tools.cmake import CMake
from conans import tools
import os

class DeviceManagementTest(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not tools.cross_building(self.settings):
            test_library = os.path.join("bin", "test_package")
            test_dm_tester_cmd = "dm-tester -h"

            for test in [test_library, test_dm_tester_cmd]:
                self.output.info(f"Running -> {test}")
                self.run(test, run_environment=True)