from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps

class CppSQLite(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "../../CMakeLists.txt", "../../src/*", "../../test/*", "../../*.cmake"

    def configure(self):

        self.options["boost"].without_wave = True
        self.options["boost"].without_type_erasure = True
        self.options["boost"].without_process = True
        self.options["boost"].without_nowide = True
        self.options["boost"].without_log = True
        self.options["boost"].without_locale = True
        self.options["boost"].without_fiber = True
        self.options["boost"].without_coroutine = True
        self.options["boost"].without_contract = True
        self.options["boost"].without_cobalt = True
        self.options["boost"].without_atomic = True
        self.options["boost"].without_chrono = True
        self.options["boost"].without_container = False
        self.options["boost"].without_filesystem = True
        self.options["boost"].without_system = False
        self.options["boost"].without_thread = True
        self.options["boost"].without_test = True

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "20"
        tc.variables["CMAKE_CXX_STANDARD_REQUIRED"] = "ON"
        
        build_type = str(self.settings.build_type).lower()
        tc.variables["CMAKE_RUNTIME_OUTPUT_DIRECTORY"] = \
            f"${{CMAKE_BINARY_DIR}}/{build_type}"
        tc.variables["CMAKE_LIBRARY_OUTPUT_DIRECTORY"] = \
            f"${{CMAKE_BINARY_DIR}}/{build_type}"
        tc.variables["CMAKE_ARCHIVE_OUTPUT_DIRECTORY"] = \
            f"${{CMAKE_BINARY_DIR}}/{build_type}"

        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def requirements(self):
        # Regular dependency for the library/app
        self.requires('sqlite3/3.47.0')
        self.requires("boost/1.86.0")
        self.requires('spdlog/1.14.1')
        # Test-only dependency
        self.test_requires("gtest/1.14.0")

    def layout(self):
        cmake_layout(self)

    def package_info(self):
        self.cpp_info.libs = ["cpp_sqlite"]  # Adjust based on actual library names