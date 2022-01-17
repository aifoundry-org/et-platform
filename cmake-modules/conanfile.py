from conans import ConanFile
import os
class CMakeModulesConan(ConanFile):
    name = "cmake-modules"
    version = "0.5.0"
    license = "esperanto"
    author = "Pau Farre <pau.farre@esperantotech.com>"
    url = "https://gitlab.esperanto.ai/software/cmake-modules"
    description = "A collection of cmake modules"
    topics = ("cmake", "modules")

    no_copy_source = True
    exports_sources = "*.cmake"

    def package(self):
        self.copy(pattern="*.cmake", dst="cmake")

    def package_info(self):
        build_modules = [
            os.path.join("cmake", "CommonProjectSettings.cmake"),
            os.path.join("cmake", "CompilerCache.cmake"),
            os.path.join("cmake", "CompilerOptions.cmake"),
            os.path.join("cmake", "CompilerSanitizers.cmake"),
            os.path.join("cmake", "CompilerWarnings.cmake"),
            os.path.join("cmake", "StaticAnalyzers.cmake")
        ]
        self.cpp_info.builddirs.append("cmake")
        self.cpp_info.build_modules = build_modules # DEPRECATED
        self.cpp_info.set_property("cmake_build_modules", build_modules)

    def package_id(self):
        self.info.header_only()
