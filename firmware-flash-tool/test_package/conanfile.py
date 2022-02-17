from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conans import tools
import os


class EsperantoFlashToolTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def test(self):
        # test 'headers' usage from consumer point of view
        if not tools.cross_building(self.settings):
            bin_path = os.path.join("bin", "test_package")
            self.run(bin_path, run_environment=True)
        
        # test 'tool' usage (only available in non-hader_only version)
        if not self.options["esperanto-flash-tool"].header_only:
            self.output.info("Testing esperanto_flash_tool ..")
            self.run("esperanto_flash_tool --help", run_environment=True)
            self.output.info(".. SUCCESS!")
        
