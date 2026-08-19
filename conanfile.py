import os
import re

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import load, copy
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd, can_run, default_cppstd, valid_min_cppstd
from conan.tools.scm import Version

required_conan_version = ">=2.0.15"


class MonoGameRecipe(ConanFile):
    name = "rbe"
    homepage = "https://github.com/ch0pex/rbe"
    description = "Reflexion Based Encoding is a C++ library that provides a simple and efficient way to serialize and deserialize objects using reflection."
    package_type = "header-library"
    settings = "os", "compiler", "build_type", "arch"

    license = "MIT License"
    author = "Álvaro Cabrera Barrio"
    url = "https://github.com/ch0pex/rbe"
    topics = ("reflection", "serialization", "deserialization",
              "cpp", "library", "binary", "encoding", "decoding")

    exports_sources = "CMakeLists.txt", "docs/*", "src/*", "tests/*", "cmake/*", "example/*"

    @property
    def _build_all(self):
        return bool(self.conf.get("user.rbe.build:all", default=False))

    @property
    def _compatibility(self):
        return {
            "gcc": "16",
            "clang": None,
            "msvc": None,
        }
    
    def set_version(self):
        content = load(self, os.path.join(self.recipe_folder, "src/CMakeLists.txt"))
        version = re.search(
            r"project\([^\)]+VERSION (\d+\.\d+\.\d+)[^\)]*\)", content
        ).group(1)
        self.version = version.strip()

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.absolute_paths = True  # only needed for CMake CI
        if self._build_all:
            tc.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
            tc.cache_variables["CMAKE_COMPILE_WARNING_AS_ERROR"] = True
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder=None if self._build_all else "src")
        if self._build_all:
            cmake.build()
            if can_run(self):
                cmake.ctest(cli_args=["--output-on-failure"])

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.cxxflags = ["-freflection", "-Wno-attributes"]

    def validate(self):
        check_min_cppstd(self, "26")

        compiler = str(self.settings.compiler)
        minimum_version = self._compatibility.get(compiler)

        if minimum_version is None:
            supported = ", ".join(sorted(self._compatibility.keys()))
            raise ConanInvalidConfiguration(
                f"{self.ref} requires C++26 static reflection, which {compiler} "
                f"does not currently support in any released version. "
                f"Supported compilers: {supported}."
            )

        compiler_version = Version(self.settings.compiler.version)
        if compiler_version < minimum_version:
            raise ConanInvalidConfiguration(
                f"{self.ref} requires {compiler} >= {minimum_version} to support "
                f"C++26 static reflection (found {compiler_version})."
            )

    def package_id(self):
        self.info.clear()

    def requirements(self):
        pass

    def build_requirements(self):
        if self._build_all:
            self.test_requires("doctest/2.4.11")
