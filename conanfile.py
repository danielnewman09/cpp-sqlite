from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
import os

class CppSQLite(ConanFile):
    name = "cpp_sqlite"
    version = "0.1.0"
    license = "MIT"
    author = "Daniel Newman"
    url = "https://github.com/danielnewman/cpp-sqlite"
    description = "A modern C++20 SQLite wrapper using Boost.Describe for reflection"
    topics = ("sqlite", "database", "cpp20", "boost")

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    # Include source files for debugging support
    exports_sources = "CMakeLists.txt", "Config.cmake.in", "cpp_sqlite/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

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

        # Note: cmake.install() handles all header installation via CMakeLists.txt
        # Headers are installed to: include/cpp_sqlite/src/cpp_sqlite/*.hpp
        #                           include/cpp_sqlite/src/utils/*.hpp

        # Copy source files for debugging support (allows stepping into library code)
        copy(self, "*.cpp",
             src=os.path.join(self.source_folder, "cpp_sqlite", "src"),
             dst=os.path.join(self.package_folder, "src"),
             keep_path=True)

    def requirements(self):
        # Regular dependency for the library/app
        self.requires('sqlite3/3.47.0')
        self.requires("boost/1.86.0")
        self.requires('spdlog/1.14.1')
        # Test-only dependency
        self.test_requires("gtest/1.14.0")

    def build_requirements(self):
        self.tool_requires("cmake/3.22.6")

    def layout(self):
        cmake_layout(self)

    def package_info(self):
        self.cpp_info.libs = ["cpp_sqlite"]
        self.cpp_info.includedirs = ["include"]

        # Set C++ standard requirement
        self.cpp_info.cxxflags = ["-std=c++20"]

        # Propagate dependencies
        self.cpp_info.requires = ["sqlite3::sqlite3", "boost::boost", "spdlog::spdlog"]