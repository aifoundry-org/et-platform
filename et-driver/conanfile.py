from conans import ConanFile

class EtIoctlConan(ConanFile):
    name = "linuxDriver"
    version = "0.1"
    # No settings/options are necessary, this is header only
    exports_sources = "et_ioctl.h"
    no_copy_source = True

    def package(self):
        self.copy("*.h", dst="include")
    
    def package_id(self):
        self.info.header_only()
