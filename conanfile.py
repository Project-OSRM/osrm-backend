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
        self.requires("xz_utils/5.4.5")
        self.requires("zlib/1.3.1")
    
    def configure(self):
        self.options["boost"].without_python = True
        self.options["boost"].without_coroutine = True
        self.options["boost"].without_stacktrace = True
        self.options["boost"].without_cobalt = True
        self.options["bzip2"].shared = True
        self.options["xz-utils"].shared = True
        
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "20"
        tc.variables["Bzip2_ROOT"] = "${CMAKE_BINARY_DIR}"
        tc.variables["LZMA_ROOT"] = "${CMAKE_BINARY_DIR}"
        tc.variables["TBB_ROOT"] = "${CONAN_ONETBB_ROOT}"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
