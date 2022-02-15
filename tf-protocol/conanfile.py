from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conans import tools 
import os 
import re
import sys
import subprocess


class TfProtocolCOnan(ConanFile):
    name = "tf-protocol"
    url = "https://gitlab.esperanto.ai/software/tf-protocol"
    license = "Esperanto Technologies"

    settings = "os", "compiler", "arch", "build_type"

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/tf-protocol.git",
        "revision": "auto",
    }
    generator = "CMakeDeps"

    def set_version(self):
        content = tools.load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
        version = re.search(r"project\(tf-protocol VERSION \s*([\d.]+)", content).group(1)
        self.version = version.strip()
    
    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
        
    def package_id(self):
        self.info.header_only()
    
    def package_info(self):
        self.cpp_info.includedirs.append(os.path.join("include", "esperanto"))
