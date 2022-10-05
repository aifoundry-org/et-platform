from conan import ConanFile
import os

class UserspaceEtDriverConan(ConanFile):
    name = "linuxDriver"
    # No settings/options are necessary, this is header only
    exports_sources = "et_ioctl.h"
    no_copy_source = True

    def package(self):
        self.copy("*.h", dst="include")
    
    def package_id(self):
        self.info.header_only()

    def set_version(self):
        with open(os.path.join(self.recipe_folder, "VERSION"), 'r') as f:
            self.version = f.readline().strip()
