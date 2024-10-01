from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps

class OsrmConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = (
        "boost/1.85.0",
        "bzip2/1.0.8",
        "expat/2.6.2",
        "lua/5.4.6",
        "onetbb/2021.12.0",
    )
    generators = "CMakeDeps"

    def configure(self):
        self.options["boost"].without_python = True
        self.options["boost"].without_coroutine = True
        self.options["boost"].without_stacktrace = True
        self.options["boost"].without_cobalt = True
        
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "20"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
