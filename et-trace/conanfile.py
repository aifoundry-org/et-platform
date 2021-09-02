from conans import ConanFile

class EsperantoTraceConan(ConanFile):
    name = "esperantoTrace"
    version = "0.1.0"
    # No settings/options are necessary, this is header only
    exports_sources = "include/*.h"
    no_copy_source = True

    def package(self):
        self.copy("*.h")
    
    def package_id(self):
        self.info.header_only()

    def package_info(self):
        self.cpp_info.components["et_trace"].names["cmake_find_package"] = "et_trace"
        self.cpp_info.components["et_trace"].names["cmake_find_package_multi"] = "et_trace"