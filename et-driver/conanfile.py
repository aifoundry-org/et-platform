from conan import ConanFile
import os

class UserspaceEtDriverConan(ConanFile):
    name = "linuxDriver"
    # No settings/options are necessary, this is header only
    exports_sources = "et_ioctl.h"
    no_copy_source = True

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def package(self):
        self.copy("*.h", dst="include")
    
    def package_id(self):
        self.info.header_only()

    def set_version(self):
        version_file = os.path.join(self.recipe_folder, "VERSION")
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name, version_file)

