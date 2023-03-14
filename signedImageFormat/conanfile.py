from conan import ConanFile
from conan.tools.files import copy

required_conan_version = ">=1.53.0"


class SignedImageFormatConan(ConanFile):
    name = "signedImageFormat"

    scm = {
        "type": "git",
        "url": "git@https://gitlab.esperanto.ai/software/signedImageFormat.git",
        "revision": "auto"
    }

    no_copy_source = True

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)

    def package(self):
        copy(self, "*.h", self.source_folder, self.package_folder, keep_path=True)

    def package_id(self):
        self.info.clear()
