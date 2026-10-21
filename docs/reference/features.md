# Feature Matrix

What RBE supports today. RBE is experimental: the API may change in any of them.

| Symbol | Meaning |
|---|---|
| ✅ | Implemented |
| 🚧 | Partially / in progress |
| ❌ | Not implemented |

## Wirable types

| Feature | Implemented | Notes |
|---|---|---|
| Integral types | ✅ | Including `bool` and character types |
| Enumerations | ✅ | Including `std::byte` |
| Arrays | ✅ | `std::array` and bounded C arrays of integral or enumeration types. Arrays of aggregates are ❌ |
| Nested structs | ✅ | |
| Fixed-size strings | ✅ | As `char` arrays, e.g. `char[8]` padded with spaces |
| Floating point | ❌ | `float`, `double` |
| Bit-fields | ❌ | Accepted by `wirable`, but RBE's behavior with them is currently undefined |
| Base classes | ❌ | |
| Contiguous ranges | ❌ | `std::vector`, `std::inplace_vector`. By default the count travels right before the range |
| Strings | ❌ | `std::string`, `std::string_view`: contiguous ranges of `char`, always length-prefixed. Null-terminated strings are not planned |
| Optional values | ❌ | `std::optional`, needs a presence encoding on the wire |

## Annotations

| Feature | Implemented | Notes |
|---|---|---|
| Byte order | ✅ | `=rbe::little`, `=rbe::big`, `=rbe::order(native)`. Little endian is used by default |
| Alignment | ✅ | `=rbe::pack`, `=rbe::align`. Align: C++ ABI and is used by default |
| Ids | ✅ | `=rbe::id`, `=rbe::id(value)` |
| Lengths | ✅ | `=rbe::self_length`, `=rbe::frame_length`, `=rbe::payload_length`, `=rbe::header_length` |
| Counts | ✅ | `=rbe::payload_count` |
| Empty types | ✅ | `=rbe::empty` |
| Debug formatter | ✅ | `=rbe::fmt`, for `std::format` and `std::ostream` |
| Presets | ✅ | `=rbe::derive<...>`, `=rbe::pack_le`, `=rbe::pack_be`, `=rbe::debug` |
| Inheritance | ✅ | From struct to members and nested types |
| Compile-time correctness checks | ✅ | `well_annotated` |
| Correctness diagnostics | ❌ | Explains what is wrong with the annotations. Planned once `std::format` is `constexpr` |
| Bit ranges | ❌ | `=rbe::bits(msb, lsb)`. Conflict checks only, not used by the layout |
| Explicit counts | 🚧 | `=rbe::count("field")`: declared and checked by `well_annotated`, not read |
| Explicit strides | ❌ | `=rbe::stride`. Explicit stride for dynamic sized ranges |
| Range dimension | ❌ | `=rbe::dimension<count_type, stride_type>`: the wire types of a range's count and stride |

## Serialization

| Feature | Implemented | Notes |
|---|---|---|
| `serialize` | ✅ | Narrow contract serialization |
| `try_serialize` | ✅ | Wider contract: checks the buffer size |
| Custom types `rbe::custom<T>` | 🚧 | Dispatched and tested. The wire slot is `sizeof(T)`, the specialization cannot state its own size yet |
| Variable-size types | ❌ | Planned short-term|

## Deserialization

| Feature | Implemented | Notes |
|---|---|---|
| `deserialize` | ✅ | Narrow contract deserialization |
| `try_deserialize` | ✅ | Wider contract: size check for every strategy, alignment check for the in-place ones |
| Lazy deserialization | ✅ | `dsrl::proxy<T>`: fields by name, index, nested path or annotation, checked at compile time |
| Eager deserialization | ✅ | Decodes into `T` |
| In-place deserialization | ✅ | `T const&` (`in_place`) and `T&` (`in_place_mut`) |
| Custom types | 🚧 | `rbe::custom<T>`, eager and through `proxy::value()`. Not in place |
| Variable-size types | ❌ | |

## Frames

### Composition

| Feature | Implemented | Notes |
|---|---|---|
| Frame | ✅ | `frame<Header, Payload>`, recursive |
| Many | ✅ | `many<T>`: run of self-delimited, constant-length frames |
| Any | ✅ | `any<T...>`: one of several types, picked by `id`; `is<T>()`, `as<T>(strategy)` |
| Blob | ✅ | `blob`: opaque bytes |
| Custom frames | ✅ | Built on `base_frame`: the derived view supplies `make`, `delimiting_concept` and optionally `length()` |
| Delimiting classification | ✅ | At compile time: buffer, length, count, static stride, implicit, static |
| Length and distance | ✅ | `rbe::frames::length` (O(1), `constant_length`), `distance` (walks) |
| Public building blocks | ✅ | `payload_extent`, `construct_payload`, `counted_extent` and their `try_` forms |

### Lazy deserialization

| Feature | Implemented | Notes |
|---|---|---|
| Frame views | ✅ | `deserialize<Frame>` / `try_deserialize<Frame>`: views over the buffer, no strategy argument |
| Header and payload access | ✅ | Structured bindings, iteration and manual stepping over `many`, partial-buffer detection |
| Header lengths | ✅ | `frame_length`, `payload_length`, `header_length` with `is_extended` / `extension_span`, `payload_count` |
| Matching | ✅ | `match` on `any`, by value or proxy, with an `unmatched` fallback |
| Range adaptors | ✅ | `views::many`, `with_ids`, `known_ids`, `unknown_ids` |
| Flatten | ✅ | `flatten` |

### Eager deserialization

| Feature | Implemented | Notes |
|---|---|---|
| Eager matching | ✅ | `match` callbacks taking the decoded struct, `any::as<T>(eager)` |
| Whole-frame decoding | ❌ | Decoding a whole frame into a value |

### Value type

| Feature | Implemented | Notes |
|---|---|---|
| Value type | ❌ | `frame::value_type` generated from the frame's parameters |

### Serialization

| Feature | Implemented | Notes |
|---|---|---|
| Value serialization | ❌ | `serialize` of a `value_type` |
| Serialization type | ❌ | `frame::srl_type`: write header and payload straight into the buffer |

## General

| Feature | Implemented | Notes |
|---|---|---|
| Wider contracts | ✅ | `try_*` next to every narrow function: the checkable preconditions are checked |
| No exceptions | 🚧 | `-fno-exceptions`. No runtime throws, but not tested |
| Header-only | ✅ | |
| Single header | ✅ | `single_include/rbe/rbe.hpp` |
| Conan package | ✅ | |
| CMake target | ✅ | `rbe::rbe` |
| No external dependencies | ✅ | |

## Requirements

| Requirement | Version | Notes |
|---|---|---|
| C++ standard | C++26 | Static reflection and annotations |
| GCC | ≥ 16 | Built with `-freflection`. CI uses GCC 16.1; Ubuntu's 16.0.1 snapshot miscompiles reflection code |
| Clang | ❌ | No released version supports static reflection |
| MSVC | ❌ | No released version supports static reflection |
| CMake | ≥ 3.31 | |
| Conan | ≥ 2.0.15 | |
