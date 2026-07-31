# Type Concepts

RBE uses a hierarchy of type concepts to determine which serialization and deserialization algorithm to apply to each type. The concepts form a layered system: each level adds constraints that enable more optimized code paths.

---

## Introspectable

The foundational requirement. A type is **introspectable** if RBE can enumerate its member variables at compile time. This is enabled for any type that satisfies `std::meta::is_enumerable_type`. Introspection is necessary because RBE must iterate over struct fields to derive serialization logic.

---

## Wirable

A type is **wirable** if it can be serialized to and deserialized from a wire format. A type satisfies this concept if it meets any of the following:

- It is an arithmetic type (`std::is_arithmetic_v`)
- It is an enumeration type (`std::is_enum_v`)
- It has a custom serder — a user-provided or RBE-provided serialization specialization
- It is an array type, provided its element type is also wirable
- It is an aggregate class such that:
  - It has at least one member variable
  - Member variables exist in only one class within the inheritance hierarchy (inheritance is allowed, but fields cannot be split across multiple levels)
  - All member variables are wirable

> **Note:** This restriction simplifies layout computation and avoids ambiguity in field ordering across inheritance hierarchies.

---

## Trivially Wirable

A stricter subset of wirable. A type is **trivially wirable** if:

- It is wirable
- Its in-memory struct layout matches its wire layout (same field order, same sizes, no padding differences)
- It is trivially copyable (`std::is_trivially_copyable_v`)

Trivially wirable types are the fast path: they can be serialized and deserialized with a single `memcpy`.

---

## Serialization Concepts

### Serializable

Every wirable type is serializable. The serialization algorithm is selected based on the type's concept:

- **Trivially wirable** → optimized to `memcpy`
- **Wirable (non-trivial)** → field-by-field serialization with per-field transformations (endianness, padding, non-standard widths)

---

## Deserialization Concepts

Deserialization in RBE supports three modes, each with different requirements and performance characteristics.

All three modes optimize to `memcpy` when the type is **trivially wirable**. The differences between modes only affect the non-trivial path.

### Eager Deserializable

A type is **eager deserializable** if the entire message can be materialized into memory in a single pass. Requires:

- The type is trivially wirable, **or**
- The type is wirable **and**:
  - It is default constructible
  - All of its member variables are recursively eager deserializable

The recursive requirement ensures that nested structures can also be fully materialized without partial construction.

### Lazy Deserializable

A type is **lazy deserializable** if it supports on-demand field access through a lightweight buffer view. Requires:

- The type is wirable

Unlike eager deserialization, lazy deserialization does not require default constructibility. Fields are read and converted individually at access time, so the object is never fully constructed upfront.

### In-Place Deserializable

A type is **in-place deserializable** if the wire buffer can be reinterpreted directly as the C++ object with zero copy. Requires:

- The type is trivially wirable
- The type is trivially constructible and trivially destructible

This is the most restrictive mode and the fastest path: the buffer is bitcast to the target type with no allocation, no copy, and no transformation.

---

## Concept Hierarchy

```
wirable
  └── trivially_wirable
        ├── serializable (memcpy path)
        ├── eager_deserializable
        └── in_place_deserializable

wirable (non-trivial)
  ├── serializable (field-by-field path)
  ├── eager_deserializable (if default constructible + recursive check)
  └── lazy_deserializable
```
