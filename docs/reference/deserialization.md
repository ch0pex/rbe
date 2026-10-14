# Deserialization Reference

Header: `rbe/dsrl/deserialize.hpp` (strategy tags in `rbe/dsrl/tags.hpp`, lazy view type in `rbe/dsrl/proxy.hpp`)

Unlike serialization, deserialization has no single "right" strategy — the cost of materializing a message depends on how much of it you actually need. `rbe::deserialize` is a free function selected with an explicit **strategy tag** as its second argument:

```cpp
--8<-- "example/docs/reference_deserialization.cpp:signatures"
```

`buffer` is a `std::span<std::byte const>` (a mutable `std::span<std::byte>` for `in_place_mut`). There is no default strategy — it must always be picked explicitly at the call site.

## Strategies

| Strategy | Tag value | Requires | Returns | Copies buffer? | Endianness translated? |
|---|---|---|---|---|---|
| Eager | `rbe::dsrl::eager` | `wirable`, default-constructible | `T` by value | Yes — full object materialized upfront | Yes, per field |
| Lazy | `rbe::dsrl::lazy` | `wirable` | `rbe::dsrl::proxy<T>` (view) | No — fields decoded on access | Yes, per field, at access time |
| In-place | `rbe::dsrl::in_place` / `rbe::dsrl::in_place_mut` | `trivially_wirable` | `T const&` / `T&` | No — buffer reinterpreted via `std::start_lifetime_as` | No |

All three strategies collapse to the same fast path for [trivially wirable](../explanation/concepts.md#trivially-wirable) types — the differences below only matter for non-trivial types (custom endianness, non-trivial nesting, etc).

### Eager

Fully materializes `T`. Every member is read and endianness-converted immediately; the result owns its data independently of the buffer.

```cpp
--8<-- "example/docs/reference_deserialization.cpp:eager"
```

Use it when you need to keep the message around after the buffer is reused or freed, or when you're going to touch every field anyway.

### Lazy

Returns an `rbe::dsrl::proxy<T>` — a lightweight proxy that borrows the buffer and decodes a member only when it is asked for:

```cpp
--8<-- "example/docs/reference_deserialization.cpp:lazy"
```

`field<Name>()` looks the member up by identifier at compile time (a missing field is a compile error, satisfying the "no silent typos" requirement); `field<Index>()` looks it up by position. Neither call touches any other member, and no copy of `T` is ever made.

`dsrl::proxy<T>` borrows the buffer it was constructed from — the buffer must stay valid for as long as the view is used. It's currently only available for types that are not `custom_wirable`.

Use lazy when you only need a handful of fields out of a message (e.g. routing/filtering on a header before deciding whether to decode the payload at all), or when avoiding a full copy matters for throughput.

### In-place

Reinterprets the buffer directly as `T const&` (or `T&` for a mutable, non-const buffer via `in_place_mut`), with zero copies and zero allocation:

```cpp
--8<-- "example/docs/reference_deserialization.cpp:in_place"
```

Only available for [trivially wirable](../explanation/concepts.md#trivially-wirable) types — the wire layout must be bit-for-bit identical to `T`'s in-memory layout, which also means **no endianness translation happens**: the buffer's bytes are used exactly as they are. If the buffer isn't correctly aligned for `T`, behavior is undefined.

Use it when the wire format and host are known to match (e.g. same-endian same-ABI process boundary) and you want the absolute fastest path, with direct, mutable access to the underlying buffer if needed.

## Strategy tags

The four tag types (`rbe::dsrl::eager_t`, `lazy_t`, `in_place_t`, `in_place_mut_t`) and their singleton instances (`eager`, `lazy`, `in_place`, `in_place_mut`) live in `rbe::dsrl`. All of them satisfy the `rbe::dsrl::strategy` concept, which generic code can constrain against instead of listing the four types by hand.
