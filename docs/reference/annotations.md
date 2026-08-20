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
| Endianness | `little`, `big`, `native`, `bits` | at most one per annotation range |
| Alignment | `pack`, `align` | at most one per annotation range |
| Metadata | `id`, `length` | each may appear at most once across the whole (possibly nested) type |
| — (unconstrained) | `fmt` | none — freely repeatable/combinable |

Struct-level annotations are inherited by every member; a member's own annotations (or its type's annotations, for nested structs) take precedence and fully replace the inherited ones on a per-dimension basis.

---

## Endianness

Header: `rbe/annotations/endianness.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::little` | struct, member | Fields are serialized in little-endian byte order. |
| `=rbe::big` | struct, member | Fields are serialized in big-endian byte order. |
| `=rbe::native` | struct, member | Fields are serialized in the host's native byte order. This is also the implicit default when no endianness annotation is present ([REQ-077](../explanation/requirements.md#implicit-annotations)). |
| `=rbe::bits(msb, lsb)` | member | Reserved for explicit bit-range placement. Declared and included in the endianness dimension's conflict checks, but not yet consumed by layout computation — **not implemented yet**. |

## Alignment

Header: `rbe/annotations/alignment.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::pack` | struct, member | The annotated struct's members are packed on the wire with no padding between them. |
| `=rbe::align` | struct, member | Explicit opt-in to standard (non-packed) C++ alignment. Functionally equivalent to omitting an alignment annotation; provided so that alignment can be stated explicitly, e.g. to override an inherited `pack`. |

## Metadata

Header: `rbe/annotations/metadata.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::id` | member | Marks the field that identifies the message type. Reserved for the type-erased dispatch mechanism (`any_msg`) described in the design overview — **dispatch is not implemented yet**; today the annotation only participates in the metadata dimension's uniqueness check. |
| `=rbe::length` | member | Marks the field that encodes the wire length of the message. Currently only participates in the metadata dimension's uniqueness check — **not yet read by serialization/deserialization**. |

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

inline constexpr auto native_abi = rbe::derive<rbe::align, rbe::native>;
struct [[=native_abi]] Msg { /* ... */ };
```

RBE ships three built-in presets:

| Preset | Equivalent to | Description |
|---|---|---|
| `rbe::pack_le` | `derive<pack, little>` | Packed, little-endian. |
| `rbe::pack_be` | `derive<pack, big>` | Packed, big-endian. |
| `rbe::debug` | `derive<fmt>` | Enables the debug formatter. |
