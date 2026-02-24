import json
import os
import re
import subprocess
import textwrap

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.cmake.cmakedeps.cmakedeps import CMakeDeps
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.env.environment import _EnvVarPlaceHolder


BUILD_ROOT = "build"
""" The topmost directory in the build hierarchy. The binaries will be output in
`Release` and `Debug` subdirectories of this directory. """


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

    # all options default to None because the real defaults are set in CMakeLists.txt
    # fmt: off
    options = {
        "asan":          [None, False, True],
        "assertions":    [None, False, True],
        "ccache":        [None, False, True],
        "coverage":      [None, False, True],
        "debug_logging": [None, False, True],
        "fuzzing":       [None, False, True],
        "lto":           [None, False, True],
        "node_package":  [None, False, True],
        "sccache":       [None, False, True],
        "shared":        [None, False, True],
        "tsan":          [None, False, True],
        "ubsan":         [None, False, True],
        "cc":            [None, "ANY"],
        "clang_tidy":    [None, "ANY"],
        "cxx":           [None, "ANY"],
        "generator":     [None, "ANY"],
    }
    # fmt: on

    def _getVarValue(self, varvalues):
        """Returns var value as string, drops placeholders"""
        values = []
        for varvalue in varvalues._values:
            if varvalue is not _EnvVarPlaceHolder:
                values.append(varvalue)
        return values

    def _writeEnvSh(self, env_vars):
        """
        Usually Conan puts the environments for building and running into
        `conanbuild.sh` and `conanrun.sh` and you are supposed to source those files.
        The trouble starts when you run under Windows but use a bash shell, like we do
        on github CI.

        Setting 5 different configuration entries we can configure Conan almost but not
        quite entirely unlike the way we want.  To avoid that configuration hell we just
        bypass Conan and write the file ourselves. Here goes:
        """
        scope = env_vars._scope
        env_path = os.path.join(self.recipe_folder, BUILD_ROOT, f"conan-{scope}-env.sh")
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
        def cache(env_name, cmake_name, option):
            if env_name in os.environ:
                tc.cache_variables[cmake_name] = os.environ[env_name]
            # why != ? see: https://docs.conan.io/2/reference/conanfile/attributes.html#options
            elif option != None:  # noqa: E711
                tc.cache_variables[cmake_name] = option

        def cache_bool(name, option):
            if option != None:  # noqa: E711
                tc.cache_variables[name] = option
            elif name in os.environ:
                tc.cache_variables[name] = (
                    os.environ.get(name).lower() in boolean_true_expressions
                )

        cache_variables = {}
        generator = None

        # if `decode_matrix.py` output a `/CMakePresets.json`, copy the `cacheVariables`
        # section
        try:
            cmake_presets = os.path.join(self.recipe_folder, "CMakePresets.json")
            with open(cmake_presets, "r") as fp:
                js = json.loads(fp.read())
                preset = js["configurePresets"][0]
                cache_variables.update(preset.get("cacheVariables", {}))
                generator = preset.get("generator")
        except IOError:
            pass

        if self.options.generator != None:  # noqa: E711
            generator = str(self.options.generator)

        tc = CMakeToolchain(self, generator=generator)
        tc.cache_variables.update(cache_variables)

        # CAVEAT: MISNOMER! cache_variables end up in CMakePresets.json
        # and must be recalled with `cmake --preset conan-release`
        # they do *NOT* automatically end up as cache variables in Cmake
        # fmt: off
        cache_bool("BUILD_SHARED_LIBS",    self.options.shared)
        cache_bool("BUILD_NODE_PACKAGE",   self.options.node_package)
        cache_bool("ENABLE_ASSERTIONS",    self.options.assertions)
        cache_bool("ENABLE_COVERAGE",      self.options.coverage)
        cache_bool("ENABLE_LTO",           self.options.lto)
        cache_bool("ENABLE_CCACHE",        self.options.ccache)
        cache_bool("ENABLE_SCCACHE",       self.options.sccache)
        cache_bool("ENABLE_ASAN",          self.options.asan)
        cache_bool("ENABLE_TSAN",          self.options.tsan)
        cache_bool("ENABLE_UBSAN",         self.options.ubsan)
        cache_bool("ENABLE_FUZZING",       self.options.fuzzing)
        cache_bool("ENABLE_DEBUG_LOGGING", self.options.debug_logging)

        cache("CC",         "CMAKE_C_COMPILER",     self.options.cc)
        cache("CXX",        "CMAKE_CXX_COMPILER",   self.options.cxx)
        cache("CLANG_TIDY", "CMAKE_CXX_CLANG_TIDY", self.options.clang_tidy)
        # fmt: on

        # OSRM uses C++20
        # remove the block that would set the cpp standard
        # tc.blocks.remove("cppstd")
        tc.blocks["generic"] = OsrmGenericBlock

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

        run_vars = vre.environment().vars(self, scope="run")
        self._writeEnvSh(vbe.environment().vars(self, scope="build"))
        self._writeEnvSh(run_vars)

        # Put an environment into the well-known location `build/conan.env`
        with open(os.path.join(self.recipe_folder, BUILD_ROOT, "conan.env"), "w") as fp:
            generators_dir = _bash_path(self.folders.generators_folder)
            configure_preset = f"conan-{self.settings.build_type}".lower()
            build_preset = configure_preset
            test_preset = configure_preset
            if self.settings.os == "Windows":
                configure_preset = "conan-default"

            fp.write(f"CMAKE_CONFIGURE_PRESET_NAME={configure_preset}\n")
            fp.write(f"CMAKE_BUILD_PRESET_NAME={build_preset}\n")
            fp.write(f"CMAKE_TEST_PRESET_NAME={test_preset}\n")
            fp.write(f"CONAN_GENERATORS_DIR={generators_dir}\n")

            # for tools that do not understand CMakePresets.json like eg. cmake --install
            fp.write(f"OSRM_CONFIG={self.settings.build_type}\n")

        tc.cache_variables["USE_CONAN"] = True
        tc.generate()

    def layout(self):
        cmake_layout(self, build_folder=BUILD_ROOT)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if _getOpt("BUILD_NODE_PACKAGE") or self.options.node_package:
            subprocess.call(
                "scripts/ci/build_node_package.sh", shell=True, cwd=self.recipe_folder
            )


if __name__ == "__main__":
    # run tests
    print(_bash_path("D:\\a\\b\\c"))
