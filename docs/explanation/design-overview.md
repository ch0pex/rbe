# RBE Design Document

> This document describes RBE's intended design. Pieces that are not implemented yet are marked 🚧 **Planned**; everything else reflects the current state of the library. See [Requirements](requirements.md) for the exhaustive, numbered specification and [Type Concepts](concepts.md) for the concept hierarchy that drives dispatch between code paths.

## Core Principles

RBE is built around declarative programming. The user describes a binary protocol through plain C++ structs and annotations; the library derives all serialization and deserialization logic from those declarations at compile time.

The library imposes no runtime overhead over a hand-written implementation. When a struct is trivially serializable (all fields are trivial types with no special annotations), serialization is implemented in terms of `memcpy`.

The library only supports protocols with a **fixed field order**. Variable-order protocols and complex nested types are out of scope.

---

## Protocol Declaration

Layout is expressed through the type system rather than annotations. Field sizes and offsets are derived from the field's type; non-standard layouts use RBE-provided types instead of annotations. Annotations are reserved for protocol semantics only. See the [Annotations Reference](../reference/annotations.md) for the complete, up-to-date list.

### Struct-level annotations

| Annotation | Description |
|---|---|
| `=rbe::little` | All fields in the struct are little-endian on the wire. |
| `=rbe::big` | All fields in the struct are big-endian on the wire. |
| `=rbe::pack` | The struct is packed — no padding between fields. |

### Field-level annotations

| Annotation | Description |
|---|---|
| `=rbe::id` | Marks the field that identifies the message type. Reserved for `any_msg` type-erased dispatch (🚧 **Planned**, see below). |
| `=rbe::length` | Marks the field that encodes the wire length of the message. Not yet read by serialization/deserialization. |
| `=rbe::little` / `=rbe::big` | Per-field endianness override, takes precedence over the struct-level annotation. |

### RBE types — 🚧 Planned

Non-standard layouts and wire-specific types are meant to be expressed through types provided by the library. None of these exist yet; today, non-standard widths and variable-length fields must be handled by hand.

| Type | Description |
|---|---|
| `rbe::uint24_t`, `rbe::uint48_t`, ... | Unsigned integers of non-power-of-two byte widths. Deserialized into the smallest fitting standard integer type. |
| `rbe::padding<N>` | N bytes of padding. The field is skipped during serialization and deserialization. |
| `rbe::string<N>` | Fixed-length string of N bytes. |
| `rbe::vector<T, N>` | Fixed-capacity sequence of up to N elements of type T, with a wire-encoded length prefix. |

---

## Serialization

Serialization is **eager**: `rbe::serialize` executes immediately and writes the result into the provided output buffer, returning the number of bytes written.

```cpp
std::size_t bytes_written = rbe::serialize(buffer, msg); // single message
```

When all fields in a struct are standard trivial types (no RBE types, no endianness annotation forcing a swap), serialization is reduced to a `memcpy`.

🚧 **Planned:** a variadic overload for composing a header with one or more payloads in a single call (`rbe::serialize(buffer, header, payload)`) is not implemented yet — see Packet Composition below.

---

## Deserialization

Unlike serialization, deserialization supports three explicit strategies, selected with a tag argument:

```cpp
auto eager_msg = rbe::deserialize<cboe::AddOrder>(buffer, rbe::dsrl::eager);    // full copy, upfront
auto lazy_msg  = rbe::deserialize<cboe::AddOrder>(buffer, rbe::dsrl::lazy);     // view, reads on demand
auto& inplace  = rbe::deserialize<cboe::AddOrder>(buffer, rbe::dsrl::in_place); // zero-copy bitcast
```

- **Eager** materializes the entire object upfront; works for any wirable, default-constructible type.
- **Lazy** returns a lightweight view (`rbe::dsrl::msg<T>`) over the buffer and reads/converts each field on demand — no full copy is made:
  ```cpp
  auto length = lazy_msg.field("length"); // compile-time error if the field does not exist
  ```
- **In-place** reinterprets the buffer directly as `T` via bitcast — zero-copy, zero-allocation, but only available for [trivially wirable](concepts.md#trivially-wirable) types, and does not perform endianness translation.

Endianness translation to host byte order is applied per field at read time for the eager and lazy strategies. See [Type Concepts](concepts.md#deserialization-concepts) for the exact constraints each mode requires.

---

## Packet Composition — 🚧 Planned

RBE is meant to provide a generic packet composition type that groups a header and one or more payloads into a single unit, with a `flatten()` utility to decompose a packet back into its constituent headers and payloads. Neither exists yet.

---

## Type Erasure and Message Dispatch — `any_msg` — 🚧 Planned

When a struct carries an `=rbe::id` annotation, it is meant to participate in type-erased message dispatch through `any_msg`, which would hold a message of any registered type and support `visit`/`match` patterns so the user can process incoming messages generically without knowing the concrete type at the call site:

```cpp
rbe::any_msg<cboe::msgs> msg = receive(buffer);
msg.match(
  [](dsrl::msg<cboe::AddOrder> msg)  { /* ... */ },
  [](dsrl::msg<cboe::ReduceSize> msg) { /* ... */ }
);
```

The `=rbe::id` annotation and `rbe::msg_list`/`rbe::type_list` ([`rbe/core/message_list.hpp`](../reference/annotations.md)) already exist as building blocks, but `any_msg` itself is not implemented yet.

---

## Debugging Utilities

- **Text format** — implemented. Any type annotated `=rbe::fmt` (or `=rbe::debug`, its `derive` preset) gets a `std::format`/`std::ostream` formatter that prints every member recursively, including base classes and bit-fields (`rbe/core/fmt.hpp`).
- **Binary format** — 🚧 **Planned**. A raw byte dump of the wire representation is not implemented yet.

---

## Constraints and Non-goals

- Field order on the wire must be fixed and match the struct declaration order.
- Complex types (nested structs, pointers, non-trivial members) are not supported.
- Variable-length fields, optional fields, and repeated fields are supported within the fixed-order constraint.
- Only GCC ≥ 16 is supported today: C++26 static reflection is not yet implemented by any released Clang or MSVC.
