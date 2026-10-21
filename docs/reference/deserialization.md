# Deserialization Reference

Header: `rbe/dsrl.hpp` (`rbe/dsrl/deserialize.hpp`, strategy tags in `rbe/dsrl/tags.hpp`, the lazy view in
`rbe/dsrl/proxy.hpp`, `rbe/dsrl/return_type.hpp`)

Deserialization has no single right strategy: what a value costs to read depends on how much of it you
need. `rbe::deserialize<T>` takes the strategy as a tag, and there is no default.

```cpp
--8<-- "example/docs/reference_deserialization.cpp:signatures"
```

`buffer` is a `std::span<std::byte const>`, or a `std::span<std::byte>` for `in_place_mut`. Every form has
a narrow contract: the buffer must hold the whole value (and be aligned, for the in-place strategies), or
the behavior is undefined. [`try_deserialize`](#wider-contract) checks what it can instead.

## Strategies

| Strategy | Tag | `T` must be | Returns | Buffer copied | Byte order converted |
|---|---|---|---|---|---|
| Eager | `rbe::dsrl::eager` | `wirable`; a non-trivial class also default constructible | `T` | yes, the whole value, now | yes, per field |
| Lazy | `rbe::dsrl::lazy` | `wirable` | `rbe::dsrl::proxy<T>` | no, decoded on access | yes, per field, on access |
| In place | `rbe::dsrl::in_place` | `trivially_wirable` | `T const&` | no, the buffer is the object | no |
| In place, mutable | `rbe::dsrl::in_place_mut` | `trivially_wirable` | `T&` | no | no |

For a [trivially wirable](core.md#type-concepts) type the four strategies meet in the same `memcpy` or
reinterpretation; the differences matter for types that need a conversion.

### Eager

```cpp
--8<-- "example/docs/reference_deserialization.cpp:eager"
```

Every member is read and converted now; the result owns its data. Use it when you will touch every field,
or when the buffer will be reused or freed before you are done.

### Lazy

```cpp
--8<-- "example/docs/reference_deserialization.cpp:lazy"
```

Returns a [proxy](#proxy) that borrows the buffer and decodes a member when asked for it. The buffer must
outlive the view. Use it to route or filter on a few fields of a large message.

### In place

```cpp
--8<-- "example/docs/reference_deserialization.cpp:in_place"
```

The bytes are reinterpreted as `T` with `std::start_lifetime_as`: no copy, no conversion, which is why the
type must be trivially wirable. The buffer must be aligned for `T`. `in_place_mut` does the same over a
mutable span and returns `T&`, so writes go to the buffer.

## Wider contract

!!! info "Narrow and wider contracts"
    A function has a **narrow contract** when some of the inputs it accepts have undefined behavior: a
    partial function, in the mathematical sense. `rbe::deserialize` is one: a buffer that is too short
    compiles and runs, and the result is undefined.

    The `try_` forms have a **wider** contract, not a wide one. They check the precondition that can be
    checked, the buffer's size (and its alignment, in place), and report `std::nullopt` instead of
    faulting. What cannot be checked stays assumed: that the span points to live memory, and that the
    bytes are what the type says. The subset of inputs without undefined behavior is larger; it is not
    everything.

`rbe::try_deserialize<T>(buffer, strategy)` checks the checkable preconditions and returns
`std::optional<rbe::dsrl::return_type<S, T>>`:

```cpp
--8<-- "example/docs/reference_deserialization.cpp:try_signatures"
```

```cpp
--8<-- "example/docs/reference_deserialization.cpp:try_deserialize"
```

| Check | Strategies |
|---|---|
| `buffer.size() >= rbe::wire_size_of<T>()` | all |
| `buffer.data()` aligned to `alignof(T)` | `in_place`, `in_place_mut` |

`in_place_mut` takes a `std::span<std::byte>` and returns `std::optional<T&>`; the other three take a
read-only span.

What it does not check, and cannot: that the memory is live for as long as a view or reference is used,
and that the bytes are a `T` at all. Those remain the caller's, so the contract is still narrow, only wider.

## Proxy

`rbe::dsrl::proxy<T>` is the lazy view: a `std::span<std::byte const>` plus `T`'s wire layout, computed at
compile time. It requires `wirable<T>` or `explicitly_empty<T>`.

```cpp
--8<-- "example/docs/reference_deserialization.cpp:proxy_api"
```

| Member | Description |
|---|---|
| `value_type`, `size_type`, `buffer_type` | `T`, `std::size_t`, `std::span<std::byte const>`. |
| `static make(buffer) -> std::optional<proxy>` | The checked constructor: `nullopt` if the buffer is shorter than `wire_size_of<T>()`. |
| `explicit proxy(buffer)` | Narrow: the buffer must hold the value. |
| `field<"name">()` | The member by name. Requires `wirable_class<T>`; a name that is not a member is a compile error. |
| `field<"a", "b", ...>()` | A path through nested members: `field<"a">().field<"b">()`. |
| `field<I>()` | The member at declaration index `I`. |
| `field<Annotation>()` | The member carrying `Annotation` (`rbe::id`, `rbe::frame_length`, ...), searched into nested members. Requires `contains_annotation<T, Annotation>`. |
| `value()`, `operator*()` | The whole `T`, decoded. |
| `size()`, `static fixed_size()` | `rbe::wire_size_of<T>()`. |
| `as_span()` | Exactly the `size()` bytes the value occupies. |
| `buffer()`, `data()` | The buffer the view was built over, which may be longer than the value, and its first byte. |
| `operator==(proxy, T const&)`, `operator==(proxy, proxy)` | Compare decoded values. Requires `std::equality_comparable<T>`. |

What `field<...>()` returns depends on the member:

- an integer or enumeration: the decoded value, by value;
- a struct, `std::array` included: another `proxy` over the member's bytes. Its type carries the
  annotations the member inherits, so it is not spelled `proxy<Member>`: write `auto`, and call `value()`
  to decode it.

The view is a span: copy it freely, and keep the buffer alive for as long as any copy is read.

## Strategy tags

The four tag types `rbe::dsrl::eager_t`, `lazy_t`, `in_place_t` and `in_place_mut_t`, and their instances
`eager`, `lazy`, `in_place` and `in_place_mut`, satisfy the `rbe::dsrl::strategy` concept. Generic code
constrains on the concept and asks `rbe::dsrl::return_type<S, T>` what a strategy hands back:

```cpp
--8<-- "example/docs/reference_deserialization.cpp:tags"
```

| `S` | `return_type<S, T>` |
|---|---|
| `eager_t` | `T` |
| `lazy_t` | `rbe::dsrl::proxy<T>` |
| `in_place_t` | `T const&` |
| `in_place_mut_t` | `T&` |

## Frames

`rbe::deserialize<F>(buffer)` and `rbe::try_deserialize<F>(buffer)` for a frame `F` take **no strategy**: a
frame is always read through its view. See [Frames](frames.md#deserializing-a-frame).
