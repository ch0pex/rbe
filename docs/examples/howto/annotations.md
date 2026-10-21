# Annotate a type

Describe how a struct travels on the wire: byte order, packing, and how those choices reach nested types.
The struct itself is untouched; only the bytes RBE reads and writes change.

Source: `example/howto/annotations.cpp`, CMake target `howto_annotations`.

## Annotate the struct

An annotation on the struct applies to every member.

```cpp
--8<-- "example/howto/annotations.cpp:struct_level"
```

## Override on a member

A member's own annotation replaces the one it inherits, one dimension at a time: `crc` below is still
packed, only its byte order changes.

```cpp
--8<-- "example/howto/annotations.cpp:member_override"
```

## Nested types

A nested struct with no annotations of its own inherits the enclosing struct's. One that has its own keeps
them, so a type declared once with its wire format is written the same way wherever it appears.

```cpp
--8<-- "example/howto/annotations.cpp:nested"
```

## Presets

`rbe::pack_le` and `rbe::pack_be` cover the common pairs. `rbe::derive<...>` groups any annotations into a
value you can name and reuse.

```cpp
--8<-- "example/howto/annotations.cpp:presets"
```

## Defaults

With no annotation a type travels little-endian with its C++ layout, padding included. That makes it
*trivially wirable*: the wire bytes are the object's bytes, and every operation is a `memcpy`.

```cpp
--8<-- "example/howto/annotations.cpp:defaults"
```

## Compile-time checks

Two annotations of the same kind in the same place, or an annotation on something it cannot apply to, is a
compile error at the point of use. `rbe::well_annotated` is the check as a concept.

```cpp
--8<-- "example/howto/annotations.cpp:checks"
```

See also: [Annotations reference](../../reference/annotations.md) for every annotation and the exact
inheritance rules.
