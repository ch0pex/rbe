# RBE Design Document

## Core Principles

RBE is built around declarative programming. The user describes a binary protocol through plain C++ structs and annotations; the library derives all serialization and deserialization logic from those declarations at compile time.

The library imposes no runtime overhead over a hand-written implementation. When a struct is trivially serializable (all fields are trivial types with no special annotations), serialization is implemented in terms of `memcpy`.

The library only supports protocols with a **fixed field order**. Variable-order protocols and complex nested types are out of scope.

---

## Protocol Declaration

Layout is expressed through the type system rather than annotations. Field sizes and offsets are derived from the field's type; non-standard layouts use RBE-provided types instead of annotations. Annotations are reserved for protocol semantics only.

### Struct-level annotations

| Annotation | Description |
|---|---|
| `=rbe::little` | All fields in the struct are little-endian on the wire. |
| `=rbe::big` | All fields in the struct are big-endian on the wire. |
| `=rbe::pack` | The struct is packed — no padding between fields. |

### Field-level annotations

| Annotation | Description |
|---|---|
| `=rbe::id` | Marks the field that identifies the message type. Enables `any_msg` type erasure for this struct. |
| `=rbe::length` | Marks the field that encodes the wire length of the message. |
| `=rbe::little` / `=rbe::big` | Per-field endianness override, takes precedence over the struct-level annotation. |

### RBE types

Non-standard layouts and wire-specific types are expressed through types provided by the library:

| Type | Description |
|---|---|
| `rbe::uint24_t`, `rbe::uint48_t`, ... | Unsigned integers of non-power-of-two byte widths. Deserialized into the smallest fitting standard integer type. |
| `rbe::padding<N>` | N bytes of padding. The field is skipped during serialization and deserialization. |
| `rbe::string<N>` | Fixed-length string of N bytes. |
| `rbe::vector<T, N>` | Fixed-capacity sequence of up to N elements of type T, with a wire-encoded length prefix. |

---

## Serialization — `rbe::srl`

Serialization is **eager**: the operation executes immediately on the `serialize()` call and writes the result into the provided output buffer.

```cpp
rbe::srl::serialize(msg, out);           // single message
rbe::srl::serialize(header, payload, out); // header + payload
```

When all fields in a struct are standard trivial types (no RBE types, no endianness annotation), serialization is reduced to a `memcpy`.

---

## Deserialization — `rbe::dsrl`

Deserialization is **lazy**: `deserialize()` returns a lightweight message view over the raw buffer. Fields are read on demand, one at a time. No full copy of the message is made.

```cpp
auto msg    = rbe::dsrl::deserialize<cboe::AddOrder>(buffer);
auto length = msg.field("length"); // if field does not exist compile-time error
```

Endianness translation to host byte order is applied per field at read time.

---

## Packet Composition

RBE provides a generic packet composition type that groups a header and one or more payloads into a single unit. The `flatten()` utility decomposes a packet back into its constituent headers and payload for further processing.

---

## Type Erasure and Message Dispatch — `any_msg`

When a struct carries an `=rbe::id` annotation, it participates in type-erased message dispatch. `any_msg` holds a message of any registered type and supports `visit`/`match` patterns, allowing the user to process incoming messages generically without knowing the concrete type at the call site.

```cpp
rbe::any_msg<cboe::msgs> msg = receive(buffer);
msg.match(
  [](dsrl::msg<cboe::AddOrder> msg)  { /* ... */ }, 
  [](dsrl::msg<cboe::ReduceSize> msg) { /* ... */ }
);
```

---

## Debugging Utilities

The library provides utilities to print messages to standard output:

- **Binary format** — raw byte dump of the wire representation.
- **Text format** — human-readable field-by-field output, using field names derived from reflection.

---

## Constraints and Non-goals

- Field order on the wire must be fixed and match the struct declaration order.
- Complex types (nested structs, pointers, non-trivial members) are not supported.
- Variable-length fields, optional fields, and repeated fields are supported within the fixed-order constraint.
