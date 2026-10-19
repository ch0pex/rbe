# RBE Feature Proposals: Template & Worked Example

## Summary

Add support for variable-size types to rbe (types whose size isn't known at compile time), starting with repeating groups (runtime-sized ranges) and strings.

## Motivation

So far, every type in rbe has been fixed-size. This is limiting: real wire protocols frequently include runtime-sized arrays. Variable-size types are distinct from rbe's existing extended headers/payloads, where `sizeof(T)` is known at compile time and only the wire *length* comes from a header field. This *dynamic length* is not used during serialization of the type itself but only to know where the serialization of payload or next frame begins.

A type counts as variable-size if:

- it is a range whose size isn't known at compile time, or
- it is a class and:
  - one of its member variables is a range whose size isn't known at compile time, or
  - one of its member variables is a variable-size class type

## Design

### Range requirements

Ranges must be sized and contiguous. The two directions differ:

- **Serialize**: any sized contiguous range, owning or not (`std::vector`, `std::array`, `std::span`, ...)
- **Deserialize**: only owning ones (`std::vector`, `std::inplace_vector`, `std::array`, ...), since a view has no
  storage to fill.

A view pipeline that is not contiguous (`filter`, `transform`, ...) is materialized first with
`std::ranges::to<std::vector>()`. The prefix depends on whether the size is known at compile time
(`static_sized_range`):

- **Static-sized** (`std::array`, C arrays): no prefix and no count tag.
- **Otherwise**: at least the count as prefix; the stride stays opt-in.

### Repeating groups

A repeating group is a range of elements repeated `count` times with a `stride` between consecutive elements. `count` is always read from a field serialized earlier on the wire; `stride` defaults to the element size but can be overridden.

**Current direction** (see Decision Log): ranges don't need an explicit `count` annotation.

- A **dynamic-size** range has its size serialized as a prefix, immediately before its elements.
- A **static-size** range (fixed, compile-time length) is serialized with no size prefix.

Distinguishing static from dynamic ranges at compile time follows the technique in [P3928R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3928r0.html). It is not yet in the standard library, but it is straightforward to implement as library-internal code.

Earlier iterations (kept for reference, see Alternatives Considered) used an explicit annotation pointing at a count field:

```cpp
struct Hola { 
    int num{251525}; 
    [[=rbe::count("num")]] std::vector<int> vec; 
};
```

however this approach was redundant and therefore error-prone. That's why the current design opts for using the range's own size, serializing it as a prefix.

```cpp
struct Hola2 { 
  char padding[20];  // some other data
  std::vector<int> vec; 
};

auto view = deserialize(buffer, lazy);
std::vector<int> vec = view.field<"vec">();

auto hola2 = deserialize(buffer, eager);
auto vec2 = hola2.vec;  
```

However this brings some rigidness: the count field must be serialized before the range itself, and it must be right before the range. This suppose a limitation
on expressiveness of the library, but can easly be overcome by using explicit annotations for the count and stride, as in the earlier design. This is still possible, but not required.

```cpp
struct Hola2 { 
  int vec_count;
  char padding[20];  // some other data
  [[=rbe::count("vec_count")]] std::vector<int> vec; 
};

```

User can still define a stride comming from a field, but this is not required. The default stride is the size of the element type.

```cpp
struct Hola2 { 
  char padding[20];  // some other data
  int vec_stride;
  [[=rbe::stride("vec_stride")]] std::vector<int> vec; 
};

```

Obviously, user can define both count and stride if needed.

```cpp
struct Hola2 { 
  char padding[20];  // some other data
  int vec_stride;
  int vec_count;
  [[=rbe::count("vec_count"), =rbe::stride("vec_stride")]] std::vector<int> vec; 
};

```

A remote stride, like the inline one below, only applies to fixed-size elements.

### Inline prefix: `=rbe::dimension`

The inline prefix's wire types are named by `=rbe::dimension`, a list of tags. Without it every prefix is a
`std::uint64_t` count; most formats fix a narrower one (SBE uses `uint16`):

```cpp
struct Hola2 {
  int n;
  char padding[20];  // some other data
  [[=rbe::dimension<rbe::count16_t>]] std::vector<int> vec1;                      // prefix: count
  [[=rbe::dimension<rbe::stride16_t, rbe::count16_t>]] std::vector<int> vec2;     // prefix: stride, count
  [[=rbe::count("n"), =rbe::dimension<rbe::stride8_t>]] std::vector<int> vec3;    // remote count, prefix: stride
};
```

- **Tags**: `rbe::count_t<T>` and `rbe::stride_t<T>` give the wire type of each prefix field, with aliases
  `count8_t` .. `count64_t` and `stride8_t` .. `stride64_t` for the unsigned integer types.
- **Order**: the order of the tags is the order on the wire, so a stride-first prefix such as SBE's
  `groupSizeEncoding` is expressible.
- **Default**: a range without a count, remote or inline, has an inline count of type `std::uint64_t`. It is a
  wire type only: in memory sizes stay `std::size_t`. `std::size_t` itself is not the default because its width
  depends on the platform (4 bytes on i686, 32-bit ARM, x32), so the same value would give different bytes;
  `std::uint64_t` holds any `size()` and gives the same bytes everywhere.
- **At most one** count and one stride per range, counting both `dimension` and the remote `count`/`stride`; a
  second one is a compile-time error.

`dimension` types the library-managed inline prefix, `count("field")` / `stride("field")` point at fields the user
owns. Expressing the inline case with a remote `count` (declaring the field right before the range) is possible,
but brings back the redundancy the Decision Log dropped.

### Stride

The stride is **opt-in**: without a stride tag only the count goes on the wire. It is not on by default because a wire stride only carries
information in one case:

| Element                    | Wire stride                          |
|----------------------------|--------------------------------------|
| Fixed-size, fixed layout   | Redundant: always `wire_size_of<T>()` |
| Fixed-size, layout grows   | Useful: old readers skip new fields  |
| Variable-size              | Meaningless: compile-time error      |

With a stride, serialization writes `wire_size_of<T>()`, and deserialization requires
`stride >= wire_size_of<T>()` (checked in hardened mode) and skips the `stride - wire_size_of<T>()` trailing bytes
of each element.

### Ranges as top-level types

A contiguous range goes through the same `serialize` / `deserialize` entry points as any wirable. The range
overloads take an optional `rbe::dimension<...>` as a template parameter, the same value used in the annotation;
without it the prefix is a `std::uint64_t` count:

```cpp
auto bytes  = rbe::serialize(orders);                                  // count: std::uint64_t
auto sized  = rbe::serialize<rbe::dimension<rbe::count16_t>>(orders);
auto result = rbe::deserialize<std::vector<order>, rbe::dimension<rbe::stride16_t, rbe::count16_t>>(
    buffer, rbe::dsrl::eager);
```

The prefix is part of the format, known at compile time like an annotation, so it is a template parameter and not
a function argument (probed with GCC 16.2).

Inside a `frame`, `many` or `any`, a
range with a custom prefix is wrapped in a one-member struct.

### Deserialization modes of a type that contains ranges

The lazy proxy (`rbe::dsrl::proxy<T>`) resolves a field by its offset. Today every offset is a compile-time
constant. A range makes the offsets of the fields after it runtime values, and how expensive they are depends on
the element:

- **Fixed-size element**: the range spans `prefix + count * stride` bytes, one read away. The offsets after it cost
  O(1).
- **Variable-size element**: the range's extent is only known by measuring every element, recursively. The offsets
  after it cost O(n).

In-place (`rbe::dsrl::in_place`, a bitcast) is ruled out for any variable-size type, whatever its element: a
`std::vector` member is not trivially wirable. The fork is lazy:

| Option                       | Variable-size element | Pro               | Con                              |
|------------------------------|-----------------------|-------------------|----------------------------------|
| **A. Lazy only if O(1)**     | eager only            | simple, O(1)      | one such range removes lazy      |
| **B. Walk on every access**  | lazy                  | no restriction    | hidden O(n) per field            |
| **C. Offsets at construction** | lazy                | O(1) access       | runtime state, O(n) to build     |

C walks the range once when the proxy is built and stores the offsets after it (*layout eager, values lazy*,
framing notes §8). With A, asking for lazy on such a type is a compile-time error.

**Recommendation: A now, C as the planned extension.** A is a subset of C: the types it accepts behave identically
under C, so moving to C later breaks nothing. B is rejected because it hides an O(n) behind an accessor that reads
as O(1).

## Alternatives Considered

- **Explicit `count`/`stride` annotations** (`[[=rbe::count("field")]]`, `[[=rbe::stride(...)]]`) on every repeating group. More flexible (the count field can live anywhere in the struct), but error-prone, since nothing keeps the annotation in sync with the actual range size.
- **User- vs. library-managed count/stride.** User-managed is more flexible but riskier; library-managed is safer but less flexible, and raises the still-open question of what happens if the user also sets them manually.
- **A stride on the wire by default.** Rejected: for a fixed layout it repeats a compile-time constant in every
  range, and for variable-size elements it has no meaning.
- **Positional `=rbe::dimension<count_type, stride_type>`**. Rejected: its fixed parameter order cannot express a
  stride-first prefix (SBE's `groupSizeEncoding`). Tags keep the order free.
- **`count` / `stride` typed by value** (`[[=rbe::stride(std::uint16_t{}), =rbe::count(std::uint16_t{})]]`), reusing
  the remote annotations, with the wire order taken from the annotation order. Rejected: longer, the value is
  ignored (`count(std::uint16_t{7})` compiles), and the prefix is split over loose annotations instead of one unit.
- **Owning ranges only, in both directions.** Symmetric, but serializing a `std::span` would force a copy into a
  container first. Rejected: non-owning contiguous ranges are accepted for serialization only.
- **Any `sized_range` for serialization** (`transform` pipelines included), written element by element without the
  intermediate `std::ranges::to<std::vector>()` copy. Deferred: contiguous ranges only for now, to keep it simple.
- **Hybrid view/owning range type** (view on lvalue construction, owning on rvalue). Rejected: it needs a runtime
  discriminator in every range.
- **Pointers to mark a count field's location.** Considered for pointing at a count located elsewhere in the struct; rejected because pointers can't be used during deserialization.

## Open Questions

1. Non-contiguous and associative containers (`std::list`, `std::forward_list`, `std::map`, `std::unordered_map`, ...) are deferred. Adding them needs `std::pair` as a wirable, a policy for duplicate keys on the wire, and a note that `unordered_*` iteration order makes the bytes non-deterministic.
2. If manual count/stride annotations ever coexist with library-managed sizing, does the library override the annotation or raise an error on mismatch?
3. How should a user point to a count field that isn't located immediately before its elements, as some existing wire protocols do? Candidate: bring back an explicit `count` annotation for this case only, since pointers are ruled out (see Alternatives Considered).
4. Extensibility for variable-size elements. A stride cannot provide it; the tool would be a per-element length prefix. Deferred until a format needs it.
5. Lazy for types with variable-size-element ranges: A or C (see *Deserialization modes of a type that contains ranges*).
6. A frame whose payload is a range, where the header already carries the count (`rbe::payload_count`), would make the inline prefix redundant.
7. The count-versus-buffer check on deserialization (see *Inline prefix*): always on, or hardened only
   (REQ-042..046)? It is a trust-boundary check costing one comparison for fixed-size elements.

## Decision Log

- Support variable-size types in rbe, starting with repeating groups and strings.
- First design: explicit `[[=rbe::count(...)]]` / `[[=rbe::stride(...)]]` annotations on a raw-pointer field.
- Iterated toward a `std::vector`-backed field read through a lazy view, then toward dedicated `rbe::dynamic_group` / `rbe::static_group` types returning a `std::span`.
- **Team decision:** drop the required `count` annotation for the general case. A range's own size gets serialized as a prefix for dynamic ranges, and not at all for static ones.
- Resolved how to distinguish static vs. dynamic ranges at compile time using the technique from P3928R0 (library-internal, no dependency on standard library support).
- `=rbe::dimension<tags...>` types the inline prefix: `count_t<T>` / `stride_t<T>` tags (aliases `count16_t`, `stride8_t`, ...), wire order = tag order. The remote `count("field")` / `stride("field")` stay as the opt-in for fields located elsewhere, deferred until Open Question 2 is settled.
- The default inline count is `std::uint64_t` on the wire (not `std::size_t`, whose width depends on the platform); sizes in memory stay `std::size_t`. Deserialization checks the count against the remaining buffer before allocating.
- The stride is opt-in and limited to fixed-size elements; on a variable-size element it is a compile-time error.
- Only contiguous ranges for now. A `static_sized_range` has no prefix; any other range has at least the count.
- Contiguous ranges are top-level wirables through the usual `serialize` / `deserialize<R>`, with the default prefix. Custom prefix types are passed as an optional `rbe::dimension<...>` template parameter to the range overloads of `serialize` / `deserialize`; inside frames, a user-written one-member struct carries them. Without them the prefix is a `std::uint64_t` count, as for a member range without annotations. No `rbe::sized_range`; an annotated alias cannot carry the prefix types.
- Serialization accepts any sized contiguous range, owning or not (`std::span` included); deserialization only owning ones. Non-contiguous view pipelines are materialized with `std::ranges::to<std::vector>()` first.
