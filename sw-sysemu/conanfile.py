from conan import ConanFile
from conan.tools.build import cross_building
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import get, rmdir, rm
import os
import tempfile


required_conan_version = ">=1.52.0"


class SwSysemuConan(ConanFile):
    name = "sw-sysemu"
    url = "git@gitlab.com:esperantotech/software/sw-sysemu.git"
    homepage = "https://gitlab.com/esperantotech/software/sw-sysemu"
    description = "The functional ETSOC-1 emulator. Able to execute RISC-V instructions with Esperanto extensions"
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "lto": [True, False],
        "fvisibility": ["default", "protected", "hidden"],
        "profiling": [True, False],
        "backtrace": [True, False],
        "preload_elfs": [True, False],
        "preload_elfs_versions_device_api": [None, "ANY"],
        "preload_elfs_versions_device_minion_rt": [None, "ANY"],
        "preload_elfs_versions_device_bootloader": [None, "ANY"],
        "preload_elfs_versions_esperanto_trace": [None, "ANY"],
        "preload_elfs_versions_et_common_libs": [None, "ANY"],
        "preload_elfs_versions_etsoc_hal": [None, "ANY"],
        "preload_compression": [None, "lz4"],
        "sdk_release": [True, False],
        "with_sys_emu_exe": [True, False],
        "with_benchmarks": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "lto": False,
        "fvisibility": "default",
        "profiling": False,
        "backtrace": False,
        "preload_elfs": True,
        "preload_elfs_versions_device_api": None,
        "preload_elfs_versions_device_minion_rt": None,
        "preload_elfs_versions_device_bootloader": None,
        "preload_elfs_versions_esperanto_trace": None,
        "preload_elfs_versions_et_common_libs": None,
        "preload_elfs_versions_etsoc_hal": None,
        "preload_compression": "lz4",
        "sdk_release": False,
        "with_sys_emu_exe": True,
        "with_benchmarks": False,
    }

    python_requires = "conan-common/[>=1.1.0 <2.0.0]"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        if not self.options.preload_elfs_versions_device_api:
            del self.options.preload_elfs_versions_device_api
        if not self.options.preload_elfs_versions_device_minion_rt:
            del self.options.preload_elfs_versions_device_minion_rt
        if not self.options.preload_elfs_versions_device_bootloader:
            del self.options.preload_elfs_versions_device_bootloader
        if not self.options.preload_elfs_versions_esperanto_trace:
            del self.options.preload_elfs_versions_esperanto_trace
        if not self.options.preload_elfs_versions_et_common_libs:
            del self.options.preload_elfs_versions_et_common_libs
        if not self.options.preload_elfs_versions_etsoc_hal:
            del self.options.preload_elfs_versions_etsoc_hal

    def set_version(self):
        get_version = self.python_requires["conan-common"].module.get_version
        self.version = get_version(self, self.name)

    def export(self):
        register_scm_coordinates = self.python_requires["conan-common"].module.register_scm_coordinates
        register_scm_coordinates(self)

    def export_sources(self):
        copy_sources_if_scm_dirty = self.python_requires["conan-common"].module.copy_sources_if_scm_dirty
        copy_sources_if_scm_dirty(self)

    def requirements(self):
        self.requires("glog/0.4.0")
        self.requires("elfio/3.8")
        self.requires("lz4/1.9.3")
        if self.options.backtrace:
            self.requires("libunwind/1.5.0")
        if self.options.with_benchmarks:
            self.requires("benchmark/1.8.0")

    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")

        if self.settings.os != "Linux":
            self.output.warn("%s has only been tested under Linux. You're on your own" % self.name)

    def layout(self):
        cmake_layout(self)
        self.cpp.source.includedirs = [".", "sw-sysemu/include"]

        self.cpp.package.libs = ["sw-sysemu"]
        self.cpp.package.includedirs = ["include"]

        libsw_sysemu_comp_name = "libsw-sysemu"
        libsw_sysemu_cmake_name = "sw-sysemu"
        self.cpp.package.components[libsw_sysemu_comp_name].set_property("cmake_target_name", libsw_sysemu_cmake_name)
        self.cpp.package.components[libsw_sysemu_comp_name].requires = ["elfio::elfio", "glog::glog"]
        self.cpp.package.components[libsw_sysemu_comp_name].libs = ["sw-sysemu"]
        if self.options.backtrace:
            self.cpp.package.components[libsw_sysemu_comp_name].requires.append("libunwind::libunwind")
        self.cpp.build.components[libsw_sysemu_comp_name].requires = ["elfio::elfio", "glog::glog"]
        self.cpp.build.components[libsw_sysemu_comp_name].libs = ["sw-sysemu"]
        self.cpp.build.components[libsw_sysemu_comp_name].libdirs = ["."]
        self.cpp.source.components[libsw_sysemu_comp_name].includedirs = ["sw-sysemu/include", "sw-sysemu/include/sw-sysemu"]

    @property
    def _default_embedded_elfs_conanfile(self):
        return os.path.join(self.source_folder, "conanfile_embedded_elfs.txt")

    def _get_default_version_emmbedded_elfs(self, package):
        with open(self._default_embedded_elfs_conanfile, 'r') as fin:
            embedded_elfs_contents = fin.read()
            start = embedded_elfs_contents.find(package)
            end = embedded_elfs_contents.find('\n', start)
            version = embedded_elfs_contents[start:end].split('/')[1]
            self.output.info(f'searching {package} found start: {start}, {end} -> version: {version}')
            return version

    def _install_embedded_elfs(self, emmedded_elfs_conanfile):
        self.run(f"conan install {emmedded_elfs_conanfile} -pr:b default -pr:h baremetal-rv64-gcc8.2-release --build missing -g deploy -if={self.build_folder}")

    def source(self):
        get_sources_if_scm_pristine = self.python_requires["conan-common"].module.get_sources_if_scm_pristine
        get_sources_if_scm_pristine(self)

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()
        if not cross_building(self):
            env = VirtualRunEnv(self)
            env.generate(scope="build")

        tc = CMakeToolchain(self)
        tc.variables["PROFILING"] = self.options.profiling
        tc.variables["BACKTRACE"] = self.options.backtrace
        tc.variables["BENCHMARKS"] = self.options.with_benchmarks
        tc.variables["ENABLE_IPO"] = self.options.lto
        tc.variables["PRELOAD_LZ4"] = "lz4" is self.options.preload_compression
        if self.options.preload_elfs:
            default_emmedded_elfs_conanfile = self._default_embedded_elfs_conanfile

            default_device_api_version = self._get_default_version_emmbedded_elfs("deviceApi")
            default_device_minion_rt_version = self._get_default_version_emmbedded_elfs("device-minion-runtime")
            default_device_bootloader_version = self._get_default_version_emmbedded_elfs("device-bootloaders")
            default_esperanto_trace_version  = self._get_default_version_emmbedded_elfs("esperantoTrace")
            default_et_common_libs_version  = self._get_default_version_emmbedded_elfs("et-common-libs")
            default_etsoc_hal_version  = self._get_default_version_emmbedded_elfs("etsoc_hal")

            if self.options.get_safe('preload_elfs_versions_device_api') or\
                    self.options.get_safe('preload_Elfs_versions_device_minion_rt') or\
                    self.options.get_safe('preload_elfs_versions_device_bootloader') or\
                    self.options.get_safe('preload_elfs_versions_esperanto_trace') or\
                    self.options.get_safe('preload_elfs_versions_et_common_libs') or\
                    self.options.get_safe('preload_elfs_versions_etsoc_hal'):
                device_api_version = self.options.get_safe('preload_elfs_versions_device_api', default_device_api_version)
                device_minion_rt_version = self.options.get_safe('preload_elfs_versions_device_minion_rt', default_device_minion_rt_version)
                device_bootloader_version = self.options.get_safe('preload_elfs_versions_device_bootloader', default_device_bootloader_version)
                esperanto_trace_version = self.options.get_safe('preload_elfs_versions_esperanto_trace', default_esperanto_trace_version)
                et_common_libs_version = self.options.get_safe('preload_elfs_versions_et_common_libs', default_et_common_libs_version)
                etsoc_hal_version = self.options.get_safe('preload_elfs_versions_etsoc_hal', default_etsoc_hal_version)

                tmp_file = tempfile.NamedTemporaryFile(prefix="custom_embedded_elfs_conanfile_")
                with open(tmp_file.name, 'w+') as f:
                    f.write(f"""
[requires]
deviceApi/{device_api_version}

device-minion-runtime/{device_minion_rt_version}
device-bootloaders/{device_bootloader_version}

# overrides
esperantoTrace/{esperanto_trace_version}
et-common-libs/{et_common_libs_version}
etsoc_hal/{etsoc_hal_version}
""")
                    f.seek(0)
                    contents = f.read()
                    f.seek(0)
                    self.output.info(f'Autogenerated conanfile {tmp_file.name} with contents:\n{contents}')
                    self._install_embedded_elfs(emmedded_elfs_conanfile=tmp_file.name)
            else:
                self._install_embedded_elfs(emmedded_elfs_conanfile=default_emmedded_elfs_conanfile)

            preload_elfs_list = [
                os.path.join(self.build_folder, "device-bootloaders/lib/esperanto-fw/BootromTrampolineToBL2/BootromTrampolineToBL2.elf"),
                os.path.join(self.build_folder, "device-bootloaders/lib/esperanto-fw/ServiceProcessorBL2/fast-boot/ServiceProcessorBL2_fast-boot.elf"),
                os.path.join(self.build_folder, "device-minion-runtime/lib/esperanto-fw/MachineMinion/MachineMinion.elf"),
                os.path.join(self.build_folder, "device-minion-runtime/lib/esperanto-fw/MasterMinion/MasterMinion.elf"),
                os.path.join(self.build_folder, "device-minion-runtime/lib/esperanto-fw/WorkerMinion/WorkerMinion.elf"),
            ]
            tc.variables["PRELOAD_ELFS"] = ";".join(preload_elfs_list)
        tc.variables["SDK_RELEASE"] = self.options.sdk_release
        tc.variables["CMAKE_ASM_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_C_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_CXX_VISIBILITY_PRESET"] = self.options.fvisibility
        tc.variables["CMAKE_VISIBILITY_INLINES_HIDDEN"] = "ON" if self.options.fvisibility == "hidden" else "OFF"
        tc.variables["CMAKE_INSTALL_LIBDIR"] = "lib"
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        if not self.options.with_sys_emu_exe:
            rm(self, "sys_emu", os.path.join(self.package_folder, "bin"))

    def package_info(self):
        # utilities
        bin_path = os.path.join(self.package_folder, "bin") if self.package_folder else "bin"
        bin_ext = ".exe" if self.settings.os == "Windows" else ""

        self.output.info("Appending PATH env var with : {}".format(bin_path))
        self.env_info.PATH.append(bin_path)

        sys_emu = os.path.join(bin_path, "sys_emu" + bin_ext)
        self.output.info("Setting SYS_EMU to {}".format(sys_emu))
        self.env_info.SYS_EMU = sys_emu
