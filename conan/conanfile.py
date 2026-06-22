import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class MonoGameRecipe(ConanFile):
    name = "RBE (Reflection Based Encoding)"
    version = "0.1"
    package_type = "application"

    # Optional metadata
    license = "MIT License"
    author = "Álvaro Cabrera Barrio"
    url = ""
    description = "Reflexion Based Encoding is a C++ library that provides a simple and efficient way to serialize and deserialize objects using reflection."
    topics = ("reflection", "serialization", "deserialization",
              "cpp", "library", "binary", "encoding", "decoding")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe
    exports_sources = "CMakeLists.txt", "src/*"

    def layout(self):
        self.folders.root = ".."
        compiler_name = str(self.settings.compiler).lower()
        build_type = str(self.settings.build_type).lower()
        base_path = f"build/{compiler_name}"

        if self.settings.compiler.sanitizer:
            base_path += f"/sanitize"
        else:
            base_path += f"/{build_type}"

        self.folders.build = base_path
        self.folders.generators = f"{base_path}/generators"
        self.folders.source = "."

    def generate(self):
        deps = CMakeDeps(self)
        deps.check_components_exist = False
        deps.generate()
        tc = CMakeToolchain(self)
        tc.user_presets_path = ""
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def requirements(self):
        pass

    def build_requirements(self):
        self.test_requires("doctest/2.4.11")
