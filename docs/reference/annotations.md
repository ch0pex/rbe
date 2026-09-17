# Annotations Reference

RBE annotations are C++26 structured annotations (`[[=expr]]`) attached to a struct or to one of its non-static data members. They carry no runtime representation — they are read back through reflection at compile time to derive layout and behavior.

```cpp
struct [[=rbe::pack, =rbe::big]] Header {
  [[=rbe::little]] std::uint16_t length;
  std::uint8_t     type;
};
```

## Dimensions

Annotations are grouped into orthogonal **dimensions**. At most one annotation from a given dimension may apply to the same *annotation range* — the struct's own annotations, or a member's own annotations combined with its type's annotations. Annotations from different dimensions freely combine. See [Design Overview](../explanation/design-overview.md) and [Requirements](../explanation/requirements.md#annotation-system-requirements) for the full inheritance, override, and conflict-detection rules.

| Dimension | Annotations | Constraint |
|---|---|---|
| Endianness | `little`, `big`, `bits(msb, lsb)` (little is the implicit default) | at most one per annotation range |
| Alignment | `pack`, `align` | at most one per annotation range |
| Id | `id`, `id(value)` | at most one per annotation range, and each may appear at most once across the whole (possibly nested) type |
| Length | `frame_length`, `payload_length`, `header_length` | at most one per annotation range, and each may appear at most once across the whole (possibly nested) type |
| — (unconstrained) | `fmt` | none — freely repeatable/combinable |

Struct-level annotations are inherited by every member; a member's own annotations (or its type's annotations, for nested structs) take precedence and fully replace the inherited ones on a per-dimension basis.

---

## Endianness

Header: `rbe/annotations/endianness.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::little` | struct, member | Fields are serialized in little-endian byte order. Spelled in full: `=rbe::order(rbe::endian::order::little)`. |
| `=rbe::big` | struct, member | Fields are serialized in big-endian byte order. Spelled in full: `=rbe::order(rbe::endian::order::big)`. |
| `=rbe::bits(msb, lsb)` | member | Reserved for explicit bit-range placement. Lives in its own header, `rbe/annotations/bits.hpp`, and joins the endianness dimension from there, so it is included in its conflict checks — but it is not yet consumed by layout computation, **not implemented yet**. |

**Little-endian is the default** whenever no endianness annotation is present anywhere in scope ([REQ-077](../explanation/requirements.md#implicit-annotations)) — a fixed byte order, deliberately not the host's. A host-dependent default would mean the same program, built for a little-endian and a big-endian machine, serializes the same struct two different ways and the two cannot interoperate; the failure only appears once there are two machines involved.

If the bytes never leave the machine, the host's order can be asked for explicitly with `=rbe::order(rbe::endian::order::native)`, which is free on every target.

## Alignment

Header: `rbe/annotations/alignment.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::pack` | struct, member | The annotated struct's members are packed on the wire with no padding between them. Spelled in full: `=rbe::alignment(rbe::alignment_mode::pack)`. |
| `=rbe::align` | struct, member | Explicit opt-in to standard (non-packed) C++ alignment. Functionally equivalent to omitting an alignment annotation; provided so that alignment can be stated explicitly, e.g. to override an inherited `pack`. Spelled in full: `=rbe::alignment(rbe::alignment_mode::align)`. |

## Message id

Header: `rbe/annotations/id.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::id` | member | Marks the field the id is read from on the wire. The annotated field must be equality comparable. |
| `=rbe::id(value)` | struct | Declares the id the annotated message type is dispatched under. The value's type must be equality comparable, and is preserved as-is (`rbe::id(msg_type_t::heartbeat)` carries a `msg_type_t`, not an `int`). |

```cpp
struct hdr { [[=rbe::id]] msg_type_t type; [[=rbe::frame_length]] std::uint16_t len; };
struct [[=rbe::id(msg_type_t::heartbeat)]] Heartbeat { hdr h; std::uint32_t timestamp; };
```

The two are the halves of message identity — where the id lives on the wire, and which id a type answers to — so they share one dimension: they may not appear in the same annotation range, and neither may repeat across the whole (possibly nested) type. Nested messages keep their own `id(value)`, as in the example above.

Both are reserved for the type-erased dispatch mechanism (`any_msg`) described in the design overview — **dispatch is not implemented yet**; today they only participate in their dimension's correctness checks.

## Length

Header: `rbe/annotations/length.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::frame_length` | member | Marks the field that encodes the total frame length on the wire — header + payload. Spelled in full: `=rbe::length(rbe::length_kind::frame)`. |
| `=rbe::payload_length` | member | Marks the field that encodes the payload length — the frame minus its header. Spelled in full: `=rbe::length(rbe::length_kind::payload)`. |
| `=rbe::header_length` | member | Marks the field that encodes the header length. Spelled in full: `=rbe::length(rbe::length_kind::header)`. |

The three are independent: a message may carry any combination of them, each at most once across the whole (possibly nested) type. The annotated field must be convertible to `std::size_t`. Like `id`, they currently only participate in their dimension's uniqueness check — **none of them is read by serialization/deserialization yet**.

## Debugging

Header: `rbe/annotations/format.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::fmt` | struct | Opts the type into RBE's universal `std::format`/`std::ostream` formatter ([`rbe/core/fmt.hpp`](../explanation/concepts.md)), which prints every member recursively, including base classes and bit-fields. Not part of any dimension — it can be combined freely with any other annotation. |

## Composing annotations — `derive`

Header: `rbe/annotations/derive.hpp`

`rbe::derive<Args...>` groups several annotations into a single value, so they can be applied together with one `=` clause or reused as a named preset:

```cpp
struct [[=rbe::derive<rbe::pack, rbe::little>]] Order { /* ... */ };

inline constexpr auto packed_be = rbe::derive<rbe::pack, rbe::big>;
struct [[=packed_be]] Msg { /* ... */ };
```

RBE ships three built-in presets:

| Preset | Equivalent to | Description |
|---|---|---|
| `rbe::pack_le` | `derive<pack, little>` | Packed, little-endian. |
| `rbe::pack_be` | `derive<pack, big>` | Packed, big-endian. |
| `rbe::debug` | `derive<fmt>` | Enables the debug formatter. |

---

## Writing a new annotation

Every annotation in RBE — built-in or your own — is written the same way: a **tag** type says what the
annotation means, `rbe::detail::annotation_kind<Tag>` is the object you spell, and calling it produces the
annotation. Named annotations are aliases of such a call.

```cpp
// 1. (optional) the dimension, if the annotation conflicts with others. The dimension and the tag
//    are plumbing -- keep them out of whatever namespace your users see, as RBE keeps its own in
//    `rbe::detail`.
struct unit_dim {
  static constexpr auto kind = rbe::detail::dimension_kind::exclusive;
};

// 2. the tag: everything the annotation knows about itself, in one place
struct unit_tag {
  using dimension  = unit_dim;      // optional -- omit for an annotation that conflicts with nothing
  using value_type = time_unit;     // optional -- omit for a pure marker like rbe::fmt

  // optional: the annotation's own correctness rule, checked by `rbe::well_annotated`
  static consteval auto check(rbe::detail::annotation_info const, std::meta::info const entity) -> bool {
    return is_integral_type(rbe::detail::normalize_type(entity));
  }
};

// 3. the factory object, plus one alias per value you want to name
inline constexpr rbe::detail::annotation_kind<unit_tag> unit {};

inline constexpr auto millis = unit(time_unit::milliseconds);
inline constexpr auto micros = unit(time_unit::microseconds);
```

```cpp
struct Tick {
  [[=millis]] std::uint32_t timestamp;          // or [[=unit(time_unit::milliseconds)]]
};
```

That is the whole recipe — there is no registry to edit and no trait to specialize. Two optional members
cover the remaining cases:

| On the tag | Effect |
|---|---|
| `static constexpr bool marker = true;` | the bare factory object is an annotation too, so `unit` and `unit(v)` both work — this is how `rbe::id` and `rbe::id(value)` coexist |
| `static constexpr auto identity = rbe::detail::identity_kind::kind;` | two annotations from this tag count as the same one even with different values, for the "at most once across the whole type" rule (`rbe::id(1)` and `rbe::id(2)` are one id said twice) |
| `using value_type = rbe::detail::deduced;` | the annotation carries whatever type it is handed, instead of a fixed one |
