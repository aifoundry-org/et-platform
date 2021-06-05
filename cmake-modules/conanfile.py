from conans import ConanFile
import os
class CMakeModulesConan(ConanFile):
    name = "cmake-modules"
    version = "0.4.1"
    license = "<Put the package license here>"
    author = "Pau Farre <pau.farre@esperantotech.com>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "A collection of cmake modules"
    topics = ("cmake", "modules")

    no_copy_source = True
    exports_sources = "*.cmake"

    def package(self):
        self.copy(pattern="*.cmake", dst="cmake")

    def package_info(self):
        self.cpp_info.builddirs.append("cmake")
        self.cpp_info.build_modules = [
            os.path.join("cmake", "CommonProjectSettings.cmake"),
            os.path.join("cmake", "CompilerCache.cmake"),
            os.path.join("cmake", "CompilerOptions.cmake"),
            os.path.join("cmake", "CompilerSanitizers.cmake"),
            os.path.join("cmake", "CompilerWarnings.cmake"),
            os.path.join("cmake", "StaticAnalyzers.cmake")
        ]

    def package_id(self):
        self.info.header_only()
