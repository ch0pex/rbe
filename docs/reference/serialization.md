# Serialization Reference

Header: `rbe/srl.hpp` (`rbe/srl/serialize.hpp`)

Serialization is eager and has two forms, one per contract:

```cpp
--8<-- "example/docs/reference_serialization.cpp:signature"
```

| Function | Returns | Contract |
|---|---|---|
| `rbe::serialize(buffer, value)` | `std::size_t`, the bytes written | **Narrow.** `buffer` must hold at least `rbe::wire_size_of<T>()` bytes; otherwise the behavior is undefined. |
| `rbe::try_serialize(buffer, value)` | `std::optional<std::size_t>` | **Wide.** Checks the size first and returns `std::nullopt`, writing nothing, when the buffer is too small. |

`buffer` is a `std::span<std::byte>`; `std::vector<std::byte>`, `std::array<std::byte, N>` and byte arrays
convert to it. `value` is any [wirable](core.md#type-concepts) object. The bytes are written from offset 0
and the count returned is always `rbe::wire_size_of<T>()`, trailing padding included.

```cpp
--8<-- "example/docs/reference_serialization.cpp:example_type"

--8<-- "example/docs/reference_serialization.cpp:example_call"
```

```cpp
--8<-- "example/docs/reference_serialization.cpp:try_serialize"
```

## Dispatch

`rbe::serialize` is overloaded on the [type concept](core.md#type-concepts) `T` satisfies; there is nothing
to select by hand.

| `T` is | Behavior |
|---|---|
| `trivially_wirable`, with no byte order or packing inherited from an enclosing type | One `memcpy` of the object's bytes. |
| a `trivially_wirable_primitive` under an inherited non-default byte order | Byte-swapped and stored. |
| `custom_wirable` | `rbe::custom<T>::serialize(out, value)`, the user's specialization ([Core](core.md#custom-types)). |
| a `wirable_range` that is not trivially copyable as a whole | Element by element. |
| a `wirable_class` otherwise | Member by member, in declaration order, each at the offset its [wire layout](core.md#wire-size) gives it and with the byte order its resolved annotations give it. |

The resolution is recursive: a nested struct is serialized with the annotations it inherits, so a struct
that is trivially wirable on its own becomes member-by-member work inside a big-endian parent.

## Composing several values

Each call writes at the start of the span it is given. To write a header and its payload back to back,
advance the span by the count returned:

```cpp
--8<-- "example/docs/reference_serialization.cpp:compose"
```

## Planned

🚧 Frames are not serializable yet: `rbe::serialize(buffer, frame_value)` and a writer that appends header
and payload into the buffer are designed but not implemented. See [Frames](frames.md#planned) and the
[feature matrix](features.md#frames).
