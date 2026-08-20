# Deserialization Reference

Header: `rbe/dsrl/deserialize.hpp` (strategy tags in `rbe/dsrl/tags.hpp`, lazy view type in `rbe/dsrl/msg.hpp`)

Unlike serialization, deserialization has no single "right" strategy — the cost of materializing a message depends on how much of it you actually need. `rbe::deserialize` is a free function selected with an explicit **strategy tag** as its second argument:

```cpp
T                output = rbe::deserialize<T>(buffer, rbe::dsrl::eager);
rbe::dsrl::msg<T> view   = rbe::deserialize<T>(buffer, rbe::dsrl::lazy);
T const&          ref    = rbe::deserialize<T>(buffer, rbe::dsrl::in_place);
```

`buffer` is a `std::span<std::byte const>` (a mutable `std::span<std::byte>` for `in_place_mut`). There is no default strategy — it must always be picked explicitly at the call site.

## Strategies

| Strategy | Tag value | Requires | Returns | Copies buffer? | Endianness translated? |
|---|---|---|---|---|---|
| Eager | `rbe::dsrl::eager` | `wirable`, default-constructible | `T` by value | Yes — full object materialized upfront | Yes, per field |
| Lazy | `rbe::dsrl::lazy` | `wirable` | `rbe::dsrl::msg<T>` (view) | No — fields decoded on access | Yes, per field, at access time |
| In-place | `rbe::dsrl::in_place` / `rbe::dsrl::in_place_mut` | `trivially_wirable` | `T const&` / `T&` | No — buffer reinterpreted via `std::start_lifetime_as` | No |

All three strategies collapse to the same fast path for [trivially wirable](../explanation/concepts.md#trivially-wirable) types — the differences below only matter for non-trivial types (custom endianness, non-trivial nesting, etc).

### Eager

Fully materializes `T`. Every member is read and endianness-converted immediately; the result owns its data independently of the buffer.

```cpp
auto order = rbe::deserialize<Order>(buffer, rbe::dsrl::eager);
```

Use it when you need to keep the message around after the buffer is reused or freed, or when you're going to touch every field anyway.

### Lazy

Returns an `rbe::dsrl::msg<T>` — a lightweight proxy that borrows the buffer and decodes a member only when it is asked for:

```cpp
auto view   = rbe::deserialize<Order>(buffer, rbe::dsrl::lazy);
auto price  = view.field<"price">();      // decoded now, by field name (compile-time checked)
auto second = view.field<1>();            // decoded now, by declaration index
```

`field<Name>()` looks the member up by identifier at compile time (a missing field is a compile error, satisfying the "no silent typos" requirement); `field<Index>()` looks it up by position. Neither call touches any other member, and no copy of `T` is ever made.

`dsrl::msg<T>` borrows the buffer it was constructed from — the buffer must stay valid for as long as the view is used. It's currently only available for types that are not `custom_wirable`.

Use lazy when you only need a handful of fields out of a message (e.g. routing/filtering on a header before deciding whether to decode the payload at all), or when avoiding a full copy matters for throughput.

### In-place

Reinterprets the buffer directly as `T const&` (or `T&` for a mutable, non-const buffer via `in_place_mut`), with zero copies and zero allocation:

```cpp
Order const& order = rbe::deserialize<Order>(buffer, rbe::dsrl::in_place);
```

Only available for [trivially wirable](../explanation/concepts.md#trivially-wirable) types — the wire layout must be bit-for-bit identical to `T`'s in-memory layout, which also means **no endianness translation happens**: the buffer's bytes are used exactly as they are. If the buffer isn't correctly aligned for `T`, behavior is undefined.

Use it when the wire format and host are known to match (e.g. same-endian same-ABI process boundary) and you want the absolute fastest path, with direct, mutable access to the underlying buffer if needed.

## Strategy tags

The four tag types (`rbe::dsrl::eager_t`, `lazy_t`, `in_place_t`, `in_place_mut_t`) and their singleton instances (`eager`, `lazy`, `in_place`, `in_place_mut`) live in `rbe::dsrl`. All of them satisfy the `rbe::dsrl::strategy` concept, which generic code can constrain against instead of listing the four types by hand.

## Planned

🚧 **Type-erased dispatch — `any_msg`.** A struct annotated `=rbe::id` (see [Annotations Reference](annotations.md#metadata)) is meant to participate in dispatch through `rbe::any_msg<MessageList>`, holding a message of any registered type and exposing `visit`/`match` so callers can process an incoming buffer without knowing its concrete type upfront:

```cpp
rbe::any_msg<cboe::msgs> msg = receive(buffer);
msg.match(
  [](dsrl::msg<cboe::AddOrder> msg)   { /* ... */ },
  [](dsrl::msg<cboe::ReduceSize> msg) { /* ... */ }
);
```

The registry of dispatchable types is meant to be built from [`rbe::msg_list<T...>`](annotations.md) (`rbe/core/message_list.hpp`), which already exposes the set as `type_list`, `variant_type`, and `tuple_type`. Neither `any_msg` nor a way to iterate a `msg_list` and read back each member's `=rbe::id` / `=rbe::length` metadata (e.g. to build a dispatch table, keyed by id, at compile time) exists yet.
