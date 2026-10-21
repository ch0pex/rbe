# Installation

RBE is header-only. Installing it means making the `rbe/` headers visible to your compiler and enabling
C++26 reflection.

## Requirements

| Requirement | Version | Notes |
|---|---|---|
| Compiler | GCC ≥ 16 | The only compiler with C++26 static reflection ([P2996](https://wg21.link/p2996)) and annotations ([P3394](https://wg21.link/p3394)) today. Clang and MSVC are not supported yet. |
| Flags | `-std=c++26 -freflection -Wno-attributes` | `-Wno-attributes` silences GCC's warning on annotations it does not recognize as attributes. The CMake target and the Conan package add them for you. |
| CMake | ≥ 3.31 | Only for the CMake and Conan routes. |
| Conan | ≥ 2.0.15 | Only for the Conan route. |

!!! warning "GCC snapshots"
    CI builds with GCC 16.1. Ubuntu's `16.0.1` snapshot miscompiles reflection code; use a release build of
    GCC 16.

## Conan

Add the requirement to your `conanfile.txt` (or `conanfile.py`):

```ini
[requires]
rbe/0.0.1

[generators]
CMakeDeps
CMakeToolchain
```

Then install and consume it as a CMake package:

```bash
conan install . -b missing -s compiler=gcc -s compiler.version=16 -s compiler.cppstd=26
```

```cmake
find_package(rbe CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE rbe::rbe)
```

The `rbe::rbe` target carries the include path and the reflection flags. The recipe rejects any compiler
other than GCC 16 at install time rather than failing later in the build.

!!! tip "Single header through Conan"
    With `-o rbe/*:single_header=True` the package ships the whole library as one `rbe/rbe.hpp` instead of
    the header tree. Code written against `#include <rbe/rbe.hpp>` builds with either.

## CMake subproject

Clone or vendor the repository and add the library's own `src/` directory, which defines the `rbe` target:

```cmake
add_subdirectory(third_party/rbe/src)
target_link_libraries(my_app PRIVATE rbe::rbe)
```

Or fetch it at configure time:

```cmake
include(FetchContent)
FetchContent_Declare(rbe GIT_REPOSITORY https://github.com/ch0pex/rbe.git GIT_TAG main SOURCE_SUBDIR src)
FetchContent_MakeAvailable(rbe)
target_link_libraries(my_app PRIVATE rbe::rbe)
```

The top-level `CMakeLists.txt` of the repository also builds the tests and examples, and needs Conan for
doctest; `src/` is the library alone.

## Single header

Copy `single_include/rbe/rbe.hpp` into your include path and compile with the flags yourself:

```bash
g++ -std=c++26 -freflection -Wno-attributes -I path/to/single_include main.cpp
```

The single header is generated from `src/rbe` on every commit, so it always matches the header tree. The
granular headers (`<rbe/annotations.hpp>`, `<rbe/frames.hpp>`, ...) exist only in the tree; with the single
header everything comes from `<rbe/rbe.hpp>`.

## Verify

The smallest program that proves the toolchain works:

```cpp
--8<-- "example/docs/installation.cpp:verify"
```

Next: the [Overview](overview.md) walks through the whole library on one protocol.
