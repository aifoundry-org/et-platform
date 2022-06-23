from conans import ConanFile, tools

class SignedImageFormatConan(ConanFile):
    name = "signedImageFormat"

    scm = {
        "type": "git",
        "url": "git@https://gitlab.esperanto.ai/software/signedImageFormat.git",
        "revision": "auto"
    }

    no_copy_source = True

    python_requires = "conan-common/[>=0.5.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, self.name)
    
    def package(self):
        self.copy("*.h")

    def package_id(self):
        self.info.header_only()
