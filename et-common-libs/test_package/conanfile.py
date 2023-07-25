from pickle import TRUE
from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, CMakeToolchain
import os


class EtCommonLibsTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "VirtualRunEnv"
    test_type = "explicit"
    
    def requirements(self):
        self.requires(self.tested_reference_str)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["WITH_SP_BL"] = self.options["et-common-libs"].with_sp_bl
        tc.variables["WITH_CM_UMODE"] = self.options["et-common-libs"].with_cm_umode
        tc.variables["WITH_MINION_BL"] = self.options["et-common-libs"].with_minion_bl
        tc.variables["WITH_MM_RT_SVCS"] = self.options["et-common-libs"].with_mm_rt_svcs
        tc.variables["WITH_CM_RT_SVCS"] = self.options["et-common-libs"].with_cm_rt_svcs
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def test(self):
        tests = []
        if self.options["et-common-libs"].with_sp_bl:
            tests.append("test_package_sp_bl")
        if self.options["et-common-libs"].with_cm_umode:
            tests.append("test_package_cm_umode")
        if self.options["et-common-libs"].with_minion_bl:
            tests.append("test_package_minion_bl")
        if self.options["et-common-libs"].with_mm_rt_svcs:
            tests.append("test_package_mm_rt_svcs")
        if self.options["et-common-libs"].with_cm_rt_svcs:
            tests.append("test_package_cm_rt_svcs")

        if can_run(self):
            for test_package in tests:
                bin_path = os.path.join("bin", test_package)
                self.run(bin_path, env="conanrun")
