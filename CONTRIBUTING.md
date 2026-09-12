# Contributing to RBE

Thanks for taking the time to contribute. This document covers everything you need to build RBE from
source, run its test suite, and find your way around the repository.

## Prerequisites

| Tool | Minimum version |
|---|---|
| CMake | 3.31 |
| Conan | 2.x |
| Ninja | any recent (or another CMake generator) |
| Compiler | GCC 16+ |
| ccache *(optional)* | any |

> **Compiler support:** RBE relies on C++26 static reflection, which today is only implemented by GCC ≥ 16. The Conan recipe's `validate()` rejects Clang and MSVC outright until they support it; there's no partial/experimental path yet.

RBE is packaged as a Conan recipe (`conanfile.py` at the repo root) that generates its own CMake presets, so there is no hand-maintained `CMakePresets.json` and no separate `conan/` directory to `cd` into.

## Build and test (one shot)

This mirrors what CI runs: it builds the library, tests, and examples, then runs the test suite:

```bash
conan create . -b missing \
    -s compiler=gcc -s compiler.version=16 -s compiler.cppstd=26 \
    -c user.rbe.build:all=True
```

## Configure and build (iterating locally)

```bash
# Install dependencies and generate CMake presets. `user.rbe.build:all=True`
# is required to build tests/examples (without it, only the header-only
# package itself is configured).
conan install . -b missing -c user.rbe.build:all=True \
    -s compiler=gcc -s compiler.version=16 -s compiler.cppstd=26 -s build_type=Debug

cmake --preset conan-default
cmake --build --preset conan-debug
```

The preset names above (`conan-default` / `conan-debug`) come from Conan's `CMakeToolchain` and vary with `build_type` (e.g. `conan-release` for a Release build). Check the generated `CMakeUserPresets.json` at the repo root, or run `cmake --list-presets`, if unsure.

## Run tests

```bash
ctest --preset conan-debug --output-on-failure
```

The suite has two halves, and a change usually touches both:

- **`tests/static/`**: compile-time assertions (`static_assert`) over concepts, annotations and type
  mappings. They are "run" by compiling `unit_test_static`; a failure is a build failure.
- **`tests/runtime/`**: doctest cases over actual serialization and deserialization behavior.
- **`tests/common/`**: the annotated structs shared by both halves (`common_structs.hpp`). Prefer
  reusing a type from there over declaring a new one in a test file.

New source files must be listed explicitly: headers in `src/CMakeLists.txt`, test translation units
in the `CMakeLists.txt` of their test directory.

## Code style

`.clang-format` and `.clang-tidy` at the repo root are the source of truth; run clang-format over the
files you touch. Where a construct is unreadable when formatted (long `static_assert` type
expressions, aligned annotation tables), wrap the block in `// clang-format off` / `// clang-format on`,
as the existing tests do.

## Documentation

Documentation sources live in [`docs/`](docs/) and are published to GitHub Pages by
`.github/workflows/docs.yml` on every push to `main`. It follows the
[Diátaxis](https://diataxis.fr/) split (tutorials, how-to guides, reference, explanation), and the
navigation is defined explicitly in `zensical.toml`, so a new page has to be added there to show up
on the site.

Build the site locally with:

```bash
pip install -r requirements.txt
zensical serve
```

## Project layout

```
rbe/
├── src/rbe/            # the library (header-only)
│   ├── annotations/    # annotation definitions and their rules
│   ├── core/           # reflection, layout and wirable concepts
│   ├── dsrl/           # deserialization (eager/lazy/in-place)
│   ├── srl/            # serialization
│   └── framing/        # frame, any, many, blob
├── tests/              # static assertions + doctest suite
├── example/            # standalone usage examples (real market protocols)
├── docs/               # reference and design documentation
├── cmake/              # CMake modules
├── conanfile.py        # Conan recipe
└── CMakeLists.txt
```

## Opening a pull request

- Keep the branch focused on one change, and make sure `ctest` passes in a Debug build before pushing.
- Add or extend tests in the half that matches the change: a new concept or type mapping belongs in
  `tests/static/`, a behavioral change in `tests/runtime/`.
- If the change alters an annotation, a concept, or the framing vocabulary, update the matching page
  under `docs/reference/` or `docs/explanation/` in the same PR.
