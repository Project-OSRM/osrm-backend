import os
import re
import textwrap

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.cmake.cmakedeps.cmakedeps import CMakeDeps
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.env.environment import _EnvVarPlaceHolder


class OsrmGenericBlock:
    # Everything here is copied verbatim into the toolchain file
    template = textwrap.dedent(
        """
        # Conan was here!
        """
    )

    def context(self):
        return {}


boolean_true_expressions = ("1", "true", "on")


def _getOpt(name):
    return os.environ.get(name, "").lower() in boolean_true_expressions


def _bash_path(path):
    """Return a real path even on Windows

    D:\\a\\b\\c => /d/a/b/c
    """
    m = re.match(r"^([A-Z]):(.*)$", path)
    if m:
        drive = m.group(1).lower()
        path = f"/{drive}{m.group(2)}"
    return path.replace("\\", "/")


class OsrmConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    options = {"shared": [True, False], "node_bindings": [True, False]}

    default_options = {"shared": False, "node_bindings": False}

    def _writeEnvSh(self, env_vars):
        """
        Usually Conan puts the environments for building and running into `conanbuild.sh`
        and `conanrun.sh` and you are supposed to source those files.  The troubles start
        when we run under Windows and use a bash shell, like we do on github CI.

        With 5 different configuration entries you can configure Conan almost but not
        quite entirely unlike the way we want.  To avoid that config hell we just write
        the file ourselves.
        """

        scope = env_vars._scope
        env_path = os.path.join(self.folders.generators_folder, f"conan-{scope}-env.sh")
        with open(env_path, "w") as fp:
            for varname, varvalues in env_vars._values.items():
                values = []
                for varvalue in varvalues._values:
                    if varvalue is _EnvVarPlaceHolder:
                        values.append(f"${varname}")
                    else:
                        values.append(_bash_path(varvalue))
                line = f"{varname}={':'.join(values)}"
                print(line, file=fp)
                self.output.info(f"scope={scope}: {line}")

    def requirements(self):
        self.requires("boost/1.88.0")
        self.requires("bzip2/1.0.8")
        self.requires("expat/2.6.2")
        self.requires("lua/5.4.6")
        self.requires("onetbb/2021.12.0")
        self.requires("xz_utils/5.4.5")
        self.requires("zlib/1.3.1")

    def configure(self):
        self.options["boost"].without_python = True
        self.options["boost"].without_coroutine = True
        self.options["boost"].without_stacktrace = True
        self.options["boost"].without_cobalt = True
        # boost::locale puts obsolete libiconv in macOS PATH
        self.options["boost"].without_locale = True

        if self.options.shared or _getOpt("BUILD_SHARED_LIBS"):
            self.options["*"].shared = True
        else:
            self.options["bzip2"].shared = True
            self.options["xz_utils"].shared = True
            self.options["hwloc"].shared = True  # required by onetbb
            if self.settings.os == "Windows":
                self.options["boost"].shared = True
                self.options["onetbb"].shared = True

    def generate(self):
        tc = CMakeToolchain(self)
        # cache_variables end up in CMakePresets.json
        # and can be recalled with `cmake --preset conan-relase`
        # Note: this does not mean we are supporting all of these options yet in conan
        tc.cache_variables["USE_CONAN"] = True
        tc.cache_variables["BUILD_SHARED_LIBS"] = self.options.shared or _getOpt(
            "BUILD_SHARED_LIBS"
        )
        tc.cache_variables["BUILD_NODE_BINDINGS"] = (
            self.options.node_bindings or _getOpt("BUILD_NODE_BINDINGS")
        )
        tc.cache_variables["USE_CCACHE"] = os.environ.get("USE_CCACHE", "off")
        for i in (
            "ASSERTIONS",
            "CCACHE",
            "LTO",
            "SCCACHE",
            "ASAN",
            "UBSAN",
            "COVERAGE",
            "CLANG_TIDY",
        ):
            tc.cache_variables[f"ENABLE_{i}"] = _getOpt(f"ENABLE_{i}")

        # OSRM uses C++20
        # replace the block that would set the cpp standard with our own custom block
        # tc.blocks["cppstd"] = OsrmGenericBlock
        tc.blocks["generic"] = OsrmGenericBlock
        tc.generate()

        # add variable names compatible with the non-conan build
        # eg. "LUA_LIBRARIES" in addition to "lua_LIBRARIES"
        deps = CMakeDeps(self)
        deps.set_property("bzip2", "cmake_additional_variables_prefixes", ["BZIP2"])
        deps.set_property("expat", "cmake_additional_variables_prefixes", ["EXPAT"])
        deps.set_property("lua", "cmake_additional_variables_prefixes", ["LUA"])
        deps.generate()

        vre = VirtualRunEnv(self)
        vbe = VirtualBuildEnv(self)
        vre.generate()
        vbe.generate()

        self._writeEnvSh(vbe.environment().vars(self, scope="build"))
        self._writeEnvSh(vre.environment().vars(self, scope="run"))

        if "GITHUB_ENV" in os.environ:
            with open(os.environ["GITHUB_ENV"], "a") as fp:
                build_dir = _bash_path(self.folders.build_folder)
                generators_dir = _bash_path(self.folders.generators_folder)
                preset = f"conan-{self.settings.build_type}".lower()
                if self.settings.os == "Windows":
                    preset = "conan-default"

                fp.write(f"CONAN_BUILD_DIR={build_dir}\n")
                fp.write(f"CONAN_GENERATORS_DIR={generators_dir}\n")
                fp.write(f"CONAN_CMAKE_PRESET={preset}\n")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()


if __name__ == "__main__":
    # run tests
    print(_bash_path("D:\\a\\b\\c"))
