from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps

class CppSQLite(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "../../CMakeLists.txt", "../../src/*", "../../test/*", "../../*.cmake"

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "20"
        tc.variables["CMAKE_CXX_STANDARD_REQUIRED"] = "ON"
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
        # Test-only dependency
        self.test_requires("gtest/1.14.0")

    def layout(self):
        cmake_layout(self)

    def package_info(self):
        self.cpp_info.libs = ["cpp_sqlite"]  # Adjust based on actual library names