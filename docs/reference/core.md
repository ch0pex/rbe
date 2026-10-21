# Core Reference

Header: `rbe/core.hpp`

The layer the others build on: the concepts that say what RBE can encode, the one layout query, byte order
helpers, the debug formatter and a small overload set.

## Type concepts

Headers: `rbe/core/wirable_concepts.hpp`, `rbe/core/trivially_wirable_concepts.hpp`,
`rbe/core/wirable_primitives.hpp`

| Concept | Satisfied by |
|---|---|
| `rbe::introspectable<T>` | A type whose members reflection can enumerate (`std::meta::is_enumerable_type`). |
| `rbe::trivially_wirable_primitive<T>` | An integral type (`bool` and character types included) or an enumeration. |
| `rbe::custom_wirable<T>` | A type with an `rbe::custom<T>` specialization that is not a primitive. Reserved for the [planned](#custom-types) customization point. |
| `rbe::wirable_primitive<T>` | Either of the two above. |
| `rbe::wirable<T>` | A primitive; an array (`std::array` or bounded C array) of primitives; or a class type with no base classes, at least one member, every member wirable, and every member public unless the class is annotated [`friendly`](annotations.md#friendly). |
| `rbe::wirable_class<T>` | `wirable` and a class type. |
| `rbe::wirable_range<T>` | `wirable` and a `std::ranges::range`. |
| `rbe::trivially_wirable<T>` | `wirable`, not `custom_wirable`, and a type whose wire layout equals its in-memory layout: a primitive, an array of them, or a trivially copyable standard-layout class whose members are all trivially wirable and whose annotations change nothing (no `pack` that removes padding, no byte swap on this host). |
| `rbe::trivially_wirable_class<T>`, `rbe::trivially_wirable_range<T>` | `trivially_wirable` and a class type, or a contiguous range. |

```cpp
--8<-- "example/docs/reference_core.cpp:concepts"
```

A wirable class does not have to be an aggregate, but its members have to be public: a class with a private
member is not wirable until it is annotated [`=rbe::friendly`](annotations.md#friendly), the author's
consent to put its representation on the wire. Constructors and member functions do not matter. What a
class is does limit the strategies: the eager one needs it default constructible, and the in-place ones need
it trivially wirable, which asks for standard layout as well.

Not wirable today: floating point, arrays of classes, variable-size ranges (`std::vector`,
`std::string`), `std::optional`, base classes, bit-fields. The [feature matrix](features.md#wirable-types)
tracks them.

The predicates behind the concepts are `consteval` functions over `std::meta::info`, usable from your own
reflection code: `rbe::is_trivially_wirable_primitive(info)`, `rbe::is_custom_wirable(info)`,
`rbe::is_wirable_primitive(info)`, `rbe::is_trivially_wirable(info)`.

## Wire size

Header: `rbe/core/memory_layout.hpp`

```cpp
--8<-- "example/docs/reference_core.cpp:wire_size_of"
```

`rbe::wire_size_of<T>()` is the number of bytes `T` occupies on the wire, honoring its annotations: with
`pack`, the sum of its members' wire sizes; otherwise `sizeof(T)`; zero for an [`empty`](annotations.md#empty)
type. It is `consteval`, so it sizes arrays and feeds `static_assert`s. It is the only layout query the library exposes.

## Byte order

Header: `rbe/core/endian.hpp`, namespace `rbe::endian`

| Symbol | Description |
|---|---|
| `order` | `using order = std::endian;` the type every byte-order annotation and helper takes. |
| `to_native<From>(v)` | `v`, an integer in byte order `From`, converted to the host's order. Identity when `From` is `order::native`. |
| `native_to<To>(v)` | The same conversion, named for the other direction. |
| `load<T, O>(std::byte const* src)` | Read a `T` stored in byte order `O` at `src`. |
| `store<T, O>(std::byte* dst, T v)` | Write `v` at `dst` in byte order `O`. |

```cpp
--8<-- "example/docs/reference_core.cpp:endian"
```

All four are `constexpr` and work on integral types.

## Formatting

Header: `rbe/core/fmt.hpp`

A type annotated [`=rbe::fmt`](annotations.md#format) (or `rbe::debug`) gets a `std::formatter`
specialization and an `operator<<(std::ostream&, T const&)`. Both print the type name and every member as
`.name = value`, one per line, recursing into nested members through their own formatters, base classes
and bit-fields included.

```cpp
--8<-- "example/docs/reference_core.cpp:fmt"
```

A nested struct needs a formatter of its own: `=rbe::fmt` on it, or a `std::formatter` specialization. A
`std::formatter<std::endian>` is provided as well, printing `little` or `big`.

## Custom types

Header: `rbe/core/custom.hpp`

!!! info "Planned"
    `rbe::custom<T>` is meant to be the customization point for a type whose encoding is not the layout of
    its members. Today only the primary template is declared, and `custom_wirable` detects a specialization
    of it, but the path through it is not usable: a custom type gets `sizeof(T)` bytes on the wire with no
    way to state its own size, and only the eager strategies reach the specialization. The intended shape is
    two static functions, `serialize(std::span<std::byte>, T const&) -> std::size_t` and
    `deserialize(std::span<std::byte const>) -> T`. See the [feature matrix](features.md#serialization).

## Overload set

Header: `rbe/core/overload_set.hpp`

`rbe::overload<Fs...>` inherits the call operators of the callables it is built from, with a deduction
guide, so a set of lambdas becomes one visitor. `rbe::frames::dsrl::any::match` builds one from its
callbacks.

```cpp
--8<-- "example/docs/reference_core.cpp:overload"
```
