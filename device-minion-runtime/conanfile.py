from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conans import tools
from conans.errors import ConanInvalidConfiguration
import os
import re

def make_hash_array(strval):
    # the maximum length of the git hash is 32 bytes
    # which corresponds to the length of a SHA2-256 hash
    max_git_hash_length = 32

    result = "{ "
    hexbytes = bytes.fromhex(strval)
    for n in range(max_git_hash_length):
        if n > 0:
            result = result + ", "

        if n < len(hexbytes):
            result = result + "0x{0:02x}".format(hexbytes[n])
        else:
            result = result + "0x00"

    result = result + " }"
    return result


def make_version_array(strval):
    max_git_version_length = 112

    result = "{ "
    for n in range(max_git_version_length):
        if n > 0:
            result = result + ", "

        if n < len(strval):
            result = result + "'" + strval[n] + "'"
        else:
            result = result + "0"

    result = result + " }"
    return result

class DeviceMinionRuntimeConan(ConanFile):
    name = "device-minion-runtime"
    version = "0.0.1"
    url = "https://gitlab.esperanto.ai/software/device-minion-runtime"
    description = "minion-rt runtime"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {}
    default_options = {}

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/device-minion-runtime.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"

    def set_version(self):
        content = tools.load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
        version = re.search(r"project\(deviceMinionRuntime VERSION \s*([\d.]+)", content).group(1)
        self.version = version.strip()

    def requirements(self):
        self.requires("deviceApi/0.1.0")
        self.requires("esperantoTrace/0.1.0")
        self.requires("signedImageFormat/1.0")
        self.requires("etsoc_hal/0.1.0")
        self.requires("et-common-libs/0.0.3")

    def validate(self):
        if self.settings.arch != "rv64":
            raise ConanInvalidConfiguration("Cross-compiling to arch {} is not supported".format(self.settings.arch))

        et_common_libs = self.dependencies["et-common-libs"]
        # et-common-libs must be compiled with these components
        for flag in ["with_minion_bl", "with_mm_rt_svcs", "with_cm_rt_svcs"]:
            if not et_common_libs.options.get_safe(flag):
                raise ConanInvalidConfiguration("{0} requires {1} package with '-o {1}:{2}'".format(self.name, "et-common-libs", flag))

    def build_requirements(self):
        self.tool_requires("cmake-modules/[>=0.4.1 <1.0.0]")

    def generate(self):
        # Get the toolchains from "tools.cmake.cmaketoolchain:user_toolchain" conf at the
        # tool_requires
        user_toolchains = []
        for dep in self.dependencies.direct_build.values():
            ut = dep.conf_info["tools.cmake.cmaketoolchain:user_toolchain"]
            if ut:
                user_toolchains.append(ut)

        tc = CMakeToolchain(self)
        tc.variables["GIT_HASH_STRING"] = self.info.package_id()
        tc.variables["GIT_HASH_ARRAY"] = make_hash_array(self.info.package_id())
        tc.variables["GIT_VERSION_STRING"] = self.version
        tc.variables["GIT_VERSION_ARRAY"] = make_version_array(self.version)

        tc.variables["ENABLE_STRICT_BUILD_TYPES"] = True
        tc.variables["DEVICE_MINION_RUNTIME_DEPRECATED"] = True
        tc.variables["BUILD_DOC"] = False
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"

        tc.preprocessor_definitions["RISCV_ET_MINION"] = ""

        if user_toolchains:
            self.output.info("Applying user_toolchains: %s" % user_toolchains)
            tc.blocks["user_toolchain"].values["paths"] = user_toolchains

        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        etfw_includedir = os.path.join("include", "esperanto-fw")
        etfw_libdir = os.path.join("lib", "esperanto-fw")

        self.cpp_info.set_property("cmake_target_name", "EsperantoDeviceMinionRuntime")

        self.cpp_info.components["minion_rt_helpers_interface"].requires = ["signedImageFormat::signedImageFormat"]

        self.cpp_info.components["MachineMinion"].bindirs = [os.path.join(etfw_libdir, "MachineMinion")]
        self.cpp_info.components["MachineMinion"].requires = [
            "etsoc_hal::etsoc_hal",
            "et-common-libs::minion-bl",
            "minion_rt_helpers_interface"
        ]
        self.cpp_info.components["MasterMinion"].bindirs = [os.path.join(etfw_libdir, "MasterMinion")]
        self.cpp_info.components["MasterMinion"].requires = [
            "esperantoTrace::et_trace",
            "deviceApi::deviceApi",
            "etsoc_hal::etsoc_hal",
            "et-common-libs::mm-rt-svcs",
            "minion_rt_helpers_interface"
        ]
        self.cpp_info.components["WorkerMinion"].bindirs = [os.path.join(etfw_libdir, "WorkerMinion")]
        self.cpp_info.components["WorkerMinion"].requires = [
            "deviceApi::deviceApi",
            "et-common-libs::cm-rt-svcs",
            "esperantoTrace::et_trace",
            "minion_rt_helpers_interface"
        ]
