from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps

class OsrmConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("boost/1.85.0")
        self.requires("bzip2/1.0.8")
        self.requires("expat/2.6.2")
        self.requires("lua/5.4.6")
        self.requires("onetbb/2021.12.0")
        if self.settings.os == "Windows":
            self.requires("zlib/1.3.1")
    
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
