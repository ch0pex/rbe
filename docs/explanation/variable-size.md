# RBE Feature Proposals: Template & Worked Example

### Summary

Add support for variable-size types to rbe (types whose size isn't known at compile time), starting with repeating groups (runtime-sized ranges) and strings.

### Motivation

So far, every type in rbe has been fixed-size. This is limiting: real wire protocols frequently include runtime-sized arrays. Variable-size types are distinct from rbe's existing extended headers/payloads, where `sizeof(T)` is known at compile time and only the wire *length* comes from a header field. This *dynamic length* is not used during serialization of the type itself but only to know where the serialization of payload or next frame begins.

A type counts as variable-size if:

- it is a range whose size isn't known at compile time, or
- it is a class and:
  - one of its member variables is a range whose size isn't known at compile time, or
  - one of its member variables is a variable-size class type

### Design

#### Repeating groups

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

```cpp

#### Range requirements

To be usable as a repeating group, a C++ range currently must be:

- a sized, contiguous range (`std::vector`, `std::inplace_vector`, ...);
- dynamic (the original draft also listed `std::list`, which isn't contiguous, see Open Questions).

#### Non-owning ranges

Non-owning ranges can be used without restriction during serialization. One deserialization mode is where non-owning ranges make sense. This is flagged in Open Questions below, since the lazy interface (`rbe::dsrl::proxy<T>`, which resolves fields on demand via `field<"Name">()`) is the one that would naturally return views rather than owning containers, which doesn't match the "eager" wording in the original draft.

Possible future addition: a range type that behaves as a view when constructed from an lvalue and as owning when constructed from an rvalue or initializer list.

### Alternatives Considered

- **Explicit `count`/`stride` annotations** (`[[=rbe::count("field")]]`, `[[=rbe::stride(...)]]`) on every repeating group. More flexible (the count field can live anywhere in the struct), but error-prone, since nothing keeps the annotation in sync with the actual range size.
- **User- vs. library-managed count/stride.** User-managed is more flexible but riskier; library-managed is safer but less flexible, and raises the still-open question of what happens if the user also sets them manually.
- **Pointers to mark a count field's location.** Considered for pointing at a count located elsewhere in the struct; rejected because pointers can't be used during deserialization.

### Open Questions

1. Should non-contiguous or associative containers (`std::list`, `std::map`, `std::set`, `std::unordered_map`, `std::unordered_set`, ...) be allowed as repeating groups, or should the library stick to sized contiguous ranges only?
2. If manual count/stride annotations ever coexist with library-managed sizing, does the library override the annotation or raise an error on mismatch?
3. Confirm which deserialization mode actually requires non-owning ranges. The draft says "eager," but the lazy/proxy interface looks like the better fit (see Non-owning ranges above).
4. How should a user point to a count field that isn't located immediately before its elements, as some existing wire protocols do? Candidate: bring back an explicit `count` annotation for this case only, since pointers are ruled out (see Alternatives Considered).
5. Is a hybrid view/owning range type (view on lvalue construction, owning on rvalue/initializer-list construction) worth adding later?

### Decision Log

- Support variable-size types in rbe, starting with repeating groups and strings.
- First design: explicit `[[=rbe::count(...)]]` / `[[=rbe::stride(...)]]` annotations on a raw-pointer field.
- Iterated toward a `std::vector`-backed field read through a lazy view, then toward dedicated `rbe::dynamic_group` / `rbe::static_group` types returning a `std::span`.
- **Team decision:** drop the required `count` annotation for the general case. A range's own size gets serialized as a prefix for dynamic ranges, and not at all for static ones.
- Resolved how to distinguish static vs. dynamic ranges at compile time using the technique from P3928R0 (library-internal, no dependency on standard library support).
