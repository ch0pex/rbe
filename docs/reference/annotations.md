# Annotations Reference

Header: `rbe/annotations.hpp`

RBE annotations are C++26 annotations (`[[=expr]]`) attached to a struct or to one of its non-static data
members. They have no runtime representation: RBE reads them back through reflection at compile time to
derive the wire layout and the behavior of frames.

```cpp
--8<-- "example/docs/reference_annotations.cpp:basic"
```

## Dimensions

Annotations are grouped into **dimensions**. At most one annotation of a dimension may apply to the same
*annotation range*: the struct's own annotations, or a member's own annotations combined with those of its
type. Annotations from different dimensions combine freely. Some dimensions add a rule across the whole,
possibly nested, type.

| Dimension | Annotations | Per annotation range | Across the whole type |
|---|---|---|---|
| Endianness | `little`, `big`, `order(o)`, `bits(msb, lsb)` | at most one | |
| Alignment | `pack`, `align`, `alignment(m)` | at most one | |
| Id | `id`, `id(value)` | at most one | at most once |
| Length | `self_length` / `header_length`, `frame_length`, `payload_length`, `payload_count`, `length(k)` | at most one | each kind at most once |
| Count | `count("field")` | at most one | |
| Empty | `empty` | at most one | once, and only on the type itself |
| Format | `fmt` | at most one | once, and only on the type itself |

Struct-level annotations are inherited by every member. A member's own annotations, or its type's for a
nested struct, take precedence and replace the inherited ones one dimension at a time. See
[Annotate a type](../examples/howto/annotations.md) for the rules in action and the
[design notes](../explanation/annotation-system-v2.md) for how the system is built.

---

## Endianness

Header: `rbe/annotations/endianness.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::little` | struct, member | Little-endian byte order. Spelled in full: `=rbe::order(rbe::endian::order::little)`. |
| `=rbe::big` | struct, member | Big-endian byte order. Spelled in full: `=rbe::order(rbe::endian::order::big)`. |
| `=rbe::order(o)` | struct, member | The general form; `o` is an `rbe::endian::order`, an alias of `std::endian`. `order::native` asks for the host's order. |
| `=rbe::bits(msb, lsb)` | member | Reserved for explicit bit placement, both ends inclusive. The field must be integral and `msb` must fit in it. It joins this dimension for conflict checks but **is not consumed by the layout yet**. Header: `rbe/annotations/bits.hpp`. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:endianness"
```

**Little-endian is the default** whenever no endianness annotation is in scope
([REQ-077](../explanation/requirements.md#implicit-annotations)): a fixed byte order, deliberately not the
host's. A host-dependent default would make the same program serialize a struct two different ways on two
machines. If the bytes never leave the machine, `=rbe::order(rbe::endian::order::native)` says so and is
free on every target.

## Alignment

Header: `rbe/annotations/alignment.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::pack` | struct, member | Members are laid out on the wire with no padding between them. Spelled in full: `=rbe::alignment(rbe::alignment_mode::pack)`. |
| `=rbe::align` | struct, member | The regular C++ layout, padding included. The default; provided to state it explicitly, e.g. to override an inherited `pack`. Spelled in full: `=rbe::alignment(rbe::alignment_mode::align)`. |
| `=rbe::alignment(m)` | struct, member | The general form; `m` is an `rbe::alignment_mode`, `pack` or `align`. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:alignment"
```

`pack` propagates into nested structs that say nothing about alignment; a nested struct annotated `align`
keeps its padding inside a packed parent.

## Id

Header: `rbe/annotations/id.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::id` | member | Marks the field the id is read from on the wire. The field's type must be equality comparable. |
| `=rbe::id(value)` | struct | Declares the id the type answers to. `value` keeps its type (`rbe::id(msg_type::heartbeat)` carries a `msg_type`, not an `int`) and must be equality comparable. On a member it is an error. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:id"
```

The two are the halves of one thing, where the id lives and which id a type answers to, so they share a
dimension and neither may repeat across the whole type. Frames consume both: a header's `id` field is
`header.id()`, and `rbe::frames::any` selects the candidate whose `id(value)` matches it. See
[Frames](frames.md#any-view).

### Id concepts and queries

| Symbol | Kind | Meaning |
|---|---|---|
| `rbe::identifiable<T>` | concept | `T` declares `[[=rbe::id(value)]]`. |
| `rbe::identifying<T>` | concept | `T` has a direct field marked `[[=rbe::id]]`. Nested types are not searched. |
| `rbe::self_identifying<T>` | concept | Both of the above. |
| `rbe::id_like<T>` | concept | Equality comparable and copyable: what an id value has to be. |
| `rbe::id_of<T>()` | consteval function | The id `T` declares, with the type it was written with. Requires `identifiable<T>`. |
| `rbe::id_type_of<T>` | alias | The type of `id_of<T>()`. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:id_concepts"
```

## Length

Header: `rbe/annotations/length.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::frame_length` | member | The total length of the frame on the wire, header and payload. `=rbe::length(rbe::length_kind::frame)`. |
| `=rbe::payload_length` | member | The length of the payload alone, the frame minus its header. `=rbe::length(rbe::length_kind::payload)`. |
| `=rbe::header_length` | member | The length of the annotated type itself, so that a header may be longer on the wire than the code knows (a newer protocol version appended fields). The same object as `=rbe::self_length`: `=rbe::length(rbe::length_kind::self)`. |
| `=rbe::payload_count` | member | How many elements the payload holds, for a payload that is a run. `=rbe::length(rbe::length_kind::count)`. |
| `=rbe::length(k)` | member | The general form; `k` is an `rbe::length_kind`: `frame`, `payload`, `self`, `count`. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:length"
```

The kinds are independent: a header may carry any combination, each at most once across the whole type. The
field must be convertible to `std::size_t`. Frames read all four: they decide how a frame is
[delimited](frames.md#delimiting) and feed `header.frame_length()`, `payload_length()`, `length()` and
`payload_count()`. Outside frames, serialization and deserialization of a plain struct do not read them.

## Count

Header: `rbe/annotations/count.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::count("field")` | member | Names the earlier member that holds the number of elements of the annotated one. The annotated member must be a contiguous, owning range without a compile-time size (`std::vector`, not `std::array`), and `field` must be declared before it and convert to `std::size_t`. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:count"
```

🚧 The annotation is checked by `well_annotated` but **nothing reads it yet**: variable-size members are not
wirable. It is the vocabulary the [variable-size design](../explanation/variable-size.md) settles on.

## Empty

Header: `rbe/annotations/empty.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::empty` | struct | Declares a type with no members as deliberately empty: zero bytes on the wire. Only on the type itself, never on a member. |

| Symbol | Kind | Meaning |
|---|---|---|
| `rbe::explicitly_empty<T>` | concept | `T` is annotated `empty` and `std::is_empty_v<T>`. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:empty"
```

An empty struct is not `wirable` (it has nothing to write), so without the annotation it cannot be a payload.
With it, it is a valid `rbe::frames::any` candidate, a header-only message, and `match` hands it over by
value.

## Format

Header: `rbe/annotations/format.hpp`

| Annotation | Scope | Description |
|---|---|---|
| `=rbe::fmt` | struct | Opts the type into RBE's `std::format` / `std::ostream` formatter ([Core](core.md#formatting)), which prints every member recursively. Its own dimension, once per type and on the type only, so it combines with any wire annotation. It has no effect on the wire. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:fmt"
```

## Presets: `derive`

Header: `rbe/annotations/derive.hpp`

`rbe::derive<Args...>` groups annotations into a single value, applied with one `=` clause or named and
reused:

```cpp
--8<-- "example/docs/reference_annotations.cpp:derive"
```

| Preset | Equivalent to |
|---|---|
| `rbe::pack_le` | `derive<pack, little>` |
| `rbe::pack_be` | `derive<pack, big>` |
| `rbe::debug` | `derive<fmt>` |

## Correctness

Header: `rbe/annotations/annotation_concepts.hpp`

| Symbol | Kind | Meaning |
|---|---|---|
| `rbe::well_annotated<T>` | concept | Every rule above holds for `T` and, recursively, for its members: no annotation twice in one range, one per dimension per range, the whole-type rules of Id, Length, Empty and Format, and each annotation's own check (an integral field for `bits`, an earlier count field for `count`, ...). |
| `rbe::contains_annotation<T, A>` | concept | `A` appears somewhere in `T`, on the type, on a member or inside a nested member type. `A` may be a `derive<...>` list, in which case every element must appear. |

```cpp
--8<-- "example/docs/reference_annotations.cpp:concepts"
```

A type that fails `well_annotated` is still a type; the error surfaces where RBE is asked to use it, as a
`consteval` diagnostic that names the rule.

---

## Writing your own annotation

Every annotation in RBE is built from one recipe, and user code can follow it. The recipe names types in
`rbe::detail`, which is not stable API, so it is documented with the design notes:
[Annotation System, appendix](../explanation/annotation-system-v2.md#appendix-writing-a-new-annotation).
