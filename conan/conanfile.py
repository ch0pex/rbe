import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import load, copy
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.scm import Version


class MonoGameRecipe(ConanFile):
    name = "rbe"
    version = "0.1.0"
    package_type = "header-library"

    # Optional metadata
    license = "MIT License"
    author = "Álvaro Cabrera Barrio"
    url = ""
    description = "Reflexion Based Encoding is a C++ library that provides a simple and efficient way to serialize and deserialize objects using reflection."
    topics = ("reflection", "serialization", "deserialization",
              "cpp", "library", "binary", "encoding", "decoding")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    options = {
        "build_testing": [True, False],
        "build_examples": [True, False],
    }

    default_options = {
        "build_testing": False,
        "build_examples": False,
    }

    # Sources are located in the same place as this recipe
    def export_sources(self):
        # The path of the CMakeLists.txt and sources we want to export are one level above
        folder = os.path.join(self.recipe_folder, "..")
        copy(self, "*.txt", folder, self.export_sources_folder)
        copy(self, "src/**", folder, self.export_sources_folder)
        copy(self, "tests/**", folder, self.export_sources_folder)
        copy(self, "cmake/**", folder, self.export_sources_folder)

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
        tc.variables["BUILD_TESTING"] = self.options.build_testing
        tc.variables["BUILD_EXAMPLES"] = self.options.build_examples
        tc.user_presets_path = "build/ConanPresets.json"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.build_testing:
            cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.cxxflags = ["-freflection", "-Wno-attributes"]

    def validate(self):
        check_min_cppstd(self, "26")

        # 2. Optional but recommended: Enforce minimum compiler version for specific compilers
        compiler = self.settings.compiler
        compiler_version = Version(self.settings.compiler.version)

        if compiler == "gcc" and compiler_version < "16":
            raise ConanInvalidConfiguration(
                f"{self.ref} requires GCC >= 16 to support C++26 features. "
                f"Your profile is using GCC {compiler_version}."
            )

    def package_id(self):
        self.info.clear()

    def requirements(self):
        pass

    def build_requirements(self):
        if self.options.build_testing:
            self.test_requires("doctest/2.4.11")
