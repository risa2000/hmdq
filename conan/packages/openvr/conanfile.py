from conan import ConanFile
from conan.tools.layout import cmake_layout
from conan.tools.files import apply_conandata_patches, get, collect_libs
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps

required_conan_version = ">=1.43.0"

class OpenVrConan(ConanFile):
    name = "openvr"
    description = "OpenVR SDK"
    license = "BSD-3-Clause"
    topics = ("openvr", "vr")
    homepage = "https://github.com/ValveSoftware/openvr"
    url = "https://github.com/ValveSoftware/openvr"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
    }
    default_options = {
        "shared": False,
    }

    generators = "CMakeToolchain"
    exports_sources = ["patches/**"]

    def layout(self):
        cmake_layout(self, src_folder=f"{self.name}-{self.version}")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)
        apply_conandata_patches(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def package_id(self):
        del self.info.settings.compiler.cppstd

    def package(self):
        self.copy("LICENSE", dst="licenses", src=self.folders.source)
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.names["generator_name"] = "openvr"
        self.cpp_info.includedirs = ['include']  # Ordered list of include paths
        self.cpp_info.libs = collect_libs(self) # The libs to link against
        self.cpp_info.system_libs = []  # System libs to link against
        self.cpp_info.libdirs = ['lib']  # Directories where libraries can be found
        self.cpp_info.resdirs = ['res']  # Directories where resources, data, etc. can be found
        self.cpp_info.bindirs = ['bin']  # Directories where executables and shared libs can be found
        self.cpp_info.srcdirs = []  # Directories where sources can be found (debugging, reusing sources)
        self.cpp_info.build_modules = {}  # Build system utility module files
        if not self.options.shared:
            self.cpp_info.defines = ['OPENVR_BUILD_STATIC']  # preprocessor definitions
        else:
            self.cpp_info.defines = []  # preprocessor definitions
        self.cpp_info.cflags = []  # pure C flags
        self.cpp_info.cxxflags = []  # C++ compilation flags
        self.cpp_info.sharedlinkflags = []  # linker flags
        self.cpp_info.exelinkflags = []  # linker flags
        self.cpp_info.components = {} # Dictionary with the different components a package may have
        self.cpp_info.requires = []  # List of components from requirements
