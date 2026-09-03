# RBE — Reflection Binary Encoding

[![CI](https://github.com/ch0pex/rbe/actions/workflows/ci.yml/badge.svg)](https://github.com/ch0pex/rbe/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/ch0pex/rbe/branch/main/graph/badge.svg)](https://codecov.io/gh/ch0pex/rbe)
[![C++](https://img.shields.io/badge/C%2B%2B-26-blue.svg)](https://en.cppreference.com/w/cpp/26)
[![GCC](https://img.shields.io/badge/GCC-16%2B-blue.svg)](https://gcc.gnu.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A header-only C++ library for declarative binary serialization and deserialization via reflection-based annotations. Define your wire protocol once as plain C++ structs; RBE handles the rest.

## Motivation

Binary serialization in C++ typically forces a choice between two bad options:

| Approach | Problem |
|---|---|
| `#pragma pack` + type punning | Undefined behavior, not portable |
| Manual serialization code | Massive boilerplate that diverges from the struct definition |

RBE offers a third path: annotate your structs, and let the framework derive all serialization logic at compile time through reflection.

Originally designed for high-frequency trading systems, RBE is suitable for any performance-critical application that requires correct, maintainable binary protocol handling.

## Features

- **Declarative protocol definitions** — describe wire formats with standard C++ structs
- **Annotation-driven** — control endianness, packing, length, and message ID per field or per type
- **Zero-boilerplate** — no hand-written serialize/deserialize functions
- **Header-only** — single include, no link step

## Quick Start

### Define a protocol

```cpp
namespace cboeu {

struct [[=rbe::pack_le]] PacketHeader {
    std::uint16_t length;
    std::uint8_t  count;
    std::uint8_t  unit;
    std::uint32_t sequence;
};

struct [[=rbe::pack_le]] AddOrder {
    [[=rbe::length]] std::uint8_t  length;
    [[=rbe::id]]     std::uint8_t  message_type;
    std::uint32_t time_offset;
    std::uint32_t order_id;
    std::uint8_t  side_indicator;
    std::uint32_t quantity;
    std::uint64_t symbol;
    std::uint32_t price;
};

struct [[=rbe::pack_le]] ReduceSize {
    [[=rbe::length]] std::uint8_t  length;
    [[=rbe::id]]     std::uint8_t  message_type;
    std::uint32_t time_offset;
    std::uint64_t order_id;
    std::uint32_t cancelled_shares;
};

} // namespace cboeu
```

> The `[[=...]]` annotation on a struct must sit right after the `struct`/`class` keyword, before the type name — that's what attaches it to the class-head so reflection can see it. Putting it on the line above (`[[=...]]\nstruct Foo {...}`) instead attaches it to the *declaration*, and the annotation is silently invisible to RBE. `rbe::pack_le` is shorthand for `rbe::derive<rbe::pack, rbe::little>` (see [Annotations reference](#annotations-reference)); field annotations go directly before the member.

### Serialize and deserialize

```cpp
namespace cboe = cboeu;

std::array<std::byte, 1500> buffer{};

// Deserialize from raw buffer (lazy: reads fields on demand)
auto msg    = rbe::deserialize<cboe::AddOrder>(buffer, rbe::dsrl::lazy);
auto length = msg.field<"length">();

// Serialize to raw buffer
rbe::serialize(buffer, cboe::AddOrder{});
```

### Annotations reference

| Annotation | Scope | Effect |
|---|---|---|
| `=rbe::little` | struct, member | Little-endian byte order |
| `=rbe::big` | struct, member | Big-endian byte order |
| `=rbe::pack` | struct, member | Fields are packed without padding |
| `=rbe::align` | struct, member | Explicit standard C++ alignment (the implicit default) |
| `=rbe::length` | member | Marks the field that encodes the message length |
| `=rbe::id` | member | Marks the field that encodes the message type ID |
| `=rbe::fmt` | struct | Opts the type into RBE's `std::format`/`std::ostream` debug formatter |
| `=rbe::derive<...>` | struct, member | Groups several annotations under one `=` clause; `rbe::pack_le`, `rbe::pack_be`, and `rbe::debug` are built-in presets |

`little`/`big` and `pack`/`align` are each mutually exclusive within the same scope; `id`/`length` may each appear once per (possibly nested) type. See [`docs/reference/annotations.md`](docs/reference/annotations.md) for the full inheritance and conflict rules.

## Building

RBE is packaged as a Conan recipe (`conanfile.py` at the repo root) that generates its own CMake presets — there is no hand-maintained `CMakePresets.json` and no separate `conan/` directory to `cd` into.

### Prerequisites

| Tool | Minimum version |
|---|---|
| CMake | 3.31 |
| Conan | 2.x |
| Ninja | any recent (or another CMake generator) |
| Compiler | GCC 16+ |
| ccache *(optional)* | any |

> **Compiler support:** RBE relies on C++26 static reflection, which today is only implemented by GCC ≥ 16. The Conan recipe's `validate()` rejects Clang and MSVC outright until they support it — there's no partial/experimental path yet.

### Build and test (one shot)

This mirrors what CI runs — it builds the library, tests, and examples, then runs the test suite:

```bash
conan create . -b missing \
    -s compiler=gcc -s compiler.version=16 -s compiler.cppstd=26 \
    -c user.rbe.build:all=True
```

### Configure and build (iterating locally)

```bash
# Install dependencies and generate CMake presets. `user.rbe.build:all=True`
# is required to build tests/examples (without it, only the header-only
# package itself is configured).
conan install . -b missing -c user.rbe.build:all=True \
    -s compiler=gcc -s compiler.version=16 -s compiler.cppstd=26 -s build_type=Debug

cmake --preset conan-default
cmake --build --preset conan-debug
```

The preset names above (`conan-default` / `conan-debug`) come from Conan's `CMakeToolchain` and vary with `build_type` (e.g. `conan-release` for a Release build) — check the generated `CMakeUserPresets.json` at the repo root, or run `cmake --list-presets`, if unsure.

### Run tests

```bash
ctest --preset conan-debug --output-on-failure
```

## Project Structure

```
rbe/
├── src/rbe/rbe.hpp     # library (header-only)
├── tests/              # doctest test suite
├── example/            # standalone usage examples
├── docs/               # reference and design documentation
├── cmake/              # CMake modules
├── conanfile.py        # Conan recipe
└── CMakeLists.txt
```

## License

MIT License — see [LICENSE](LICENSE) for details.

Copyright (c) 2026 Álvaro Cabrera Barrio
