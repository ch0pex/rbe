# RBE — Reflection Binary Encoding

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
- **Sanitizer-clean** — ASan + UBSan presets included

## Quick Start

### Define a protocol

```cpp
namespace cboeu {

[[=rbe::little, =rbe::packing]]
struct PacketHeader {
    std::uint16_t length;
    std::uint8_t  count;
    std::uint8_t  unit;
    std::uint32_t sequence;
};

[[=rbe::little, =rbe::packing]]
struct AddOrder {
    [[=rbe::length]] std::uint8_t  length;
    [[=rbe::id]]     std::uint8_t  message_type;
    std::uint32_t time_offset;
    std::uint32_t order_id;
    std::uint8_t  side_indicator;
    std::uint32_t quantity;
    std::uint64_t symbol;
    std::uint32_t price;
};

[[=rbe::little, =rbe::packing]]
struct ReduceSize {
    [[=rbe::length]] std::uint8_t  length;
    [[=rbe::id]]     std::uint8_t  message_type;
    std::uint32_t time_offset;
    std::uint64_t order_id;
    std::uint32_t cancelled_shares;
};

} // namespace cboeu
```

### Serialize and deserialize

```cpp
namespace cboe = rbe::cboeu;

std::array<std::byte, 1500> buffer{};

// Deserialize from raw buffer
auto msg    = rbe::deserialize<cboe::AddOrder>(buffer);
auto length = msg.field("length");

// Serialize to raw buffer
rbe::serialize(cboe::AddOrder{}, buffer);
```

### Annotations reference

| Annotation | Scope | Effect |
|---|---|---|
| `=rbe::little` | struct | Fields are serialized in little-endian byte order |
| `=rbe::packing` | struct | Fields are packed without padding |
| `=rbe::length` | field | Marks the field that encodes the message length |
| `=rbe::id` | field | Marks the field that encodes the message type ID |

## Building

### Prerequisites

| Tool | Minimum version |
|---|---|
| CMake | 3.30 |
| Conan | 2.x |
| Ninja | any recent |
| GCC | C++26 capable (reflection support required) |
| ccache *(optional)* | any |

> **Compiler support:** RBE relies on C++ static reflection, which is currently only implemented in GCC. Clang and MSVC presets are included in the build system for future compatibility but are not supported yet.

### Install dependencies

```bash
cd conan
./install.sh          # installs Conan dependencies for the default profile
```

Or manually for a specific preset:

```bash
conan install . --profile=<your-profile> --build=missing
```

### Configure and build

```bash
# Debug build with GCC
cmake --workflow --preset Debug-gcc

# Release build with Clang
cmake --workflow --preset Release-clang

# Release build with MSVC (Windows)
cmake --workflow --preset Release-msvc
```

### Available presets

| Preset | Compiler | Type | Supported |
|---|---|---|---|
| `Debug-gcc` / `Release-gcc` / `RelWithDebInfo-gcc` | GCC | Standard | yes |
| `Sanitize-gcc` | GCC | ASan + UBSan | yes |
| `Coverage-gcc` | GCC | Coverage instrumentation | yes |
| `Debug-clang` / `Release-clang` / `RelWithDebInfo-clang` | Clang | Standard + clang-tidy | not yet |
| `Sanitize-clang` | Clang | ASan + UBSan | not yet |
| `Debug-msvc` / `Release-msvc` / `RelWithDebInfo-msvc` | MSVC | Standard | not yet |
| `Sanitize-msvc` | MSVC | Sanitize | not yet |

### Run tests

```bash
ctest --preset Debug-gcc        # or any other preset
```

## Project Structure

```
rbe/
├── src/rbe/rbe.hpp     # library (header-only)
├── tests/              # doctest test suite
├── conan/              # Conan recipe and configuration
├── cmake/              # CMake modules
├── CMakeLists.txt
└── CMakePresets.json
```

## License

MIT License — see [LICENSE](LICENSE) for details.

Copyright (c) 2026 Álvaro Cabrera Barrio
