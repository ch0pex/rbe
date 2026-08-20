# Serialization Reference

Header: `rbe/srl/serialize.hpp`

Serialization is a single, eager, free function:

```cpp
std::size_t bytes_written = rbe::serialize(buffer, value);
```

`buffer` is a `std::span<std::byte>` (or anything convertible to one, e.g. `std::vector<std::byte>`, `std::array<std::byte, N>`) sized to hold at least [`rbe::wire_size_of<T>()`](../explanation/concepts.md) bytes. `value` is any [wirable](../explanation/concepts.md#wirable) object. The call writes the wire representation into `buffer` starting at offset 0 and returns the number of bytes written.

```cpp
struct [[=rbe::derive<rbe::pack, rbe::little>]] Order {
  std::uint32_t order_id;
  std::uint32_t price;
};

Order order {.order_id = 12345, .price = 15050};

std::vector<std::byte> buffer(rbe::wire_size_of<Order>());
std::size_t n = rbe::serialize(buffer, order);
```

## Dispatch

`rbe::serialize` is overloaded; the compiler picks the right one based on which [type concept](../explanation/concepts.md) `T` satisfies — there is nothing to select explicitly:

| `T` satisfies | Behavior |
|---|---|
| [`trivially_wirable`](../explanation/concepts.md#trivially-wirable) | Direct `memcpy` of the object's bytes — no per-member work at runtime. |
| `custom_wirable` | Delegates to the user-provided `rbe::custom<T>::serialize(out, value)` specialization ([`rbe/core/custom.hpp`](../reference/annotations.md)). |
| `wirable_class` (non-trivial) | Iterates the type's non-static data members in declaration order and recursively serializes each one at the offset given by its [wire layout](../explanation/concepts.md), normalizing endianness per member first. |

Trivially wirable types are the fast path described in [Type Concepts](../explanation/concepts.md#trivially-wirable): whenever a struct's in-memory layout already matches its wire layout, the whole call collapses to one `memcpy`.

## Endianness

For the non-trivial (member-by-member) path, each member is converted to the byte order given by its resolved [endianness annotation](annotations.md#endianness) before being written — struct-level annotations apply to all members unless a member overrides them explicitly. See [Requirements → Endianness and Packing](../explanation/requirements.md#endianness-and-packing) for the exact inheritance/override rules.

## Planned

🚧 A variadic overload for composing a header with one or more payloads in a single call (`rbe::serialize(buffer, header, payload)`) is not implemented yet — see the Packet Composition section of the [Design Overview](../explanation/design-overview.md).
