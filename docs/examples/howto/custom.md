# Encode a non-aggregate type

RBE derives everything from a struct's members. A class with private state, invariants or a constructor is
not an aggregate and reflection will not enumerate it. Specializing `rbe::custom<T>` tells RBE how to write
and read it instead.

Source: `example/howto/custom.cpp`, CMake target `howto_custom`.

```cpp
--8<-- "example/howto/custom.cpp:type"
```

## The specialization

Two static functions, and the type is wirable.

```cpp
--8<-- "example/howto/custom.cpp:custom"
```

- `serialize` writes at the start of `out` and returns the bytes written. `deserialize` reads from the
  start of `in`.
- The slot the type gets inside an enclosing struct is `sizeof(T)` bytes: a custom encoding must fit in it,
  and anything it does not write is left as it was.
- The specialization has to be visible before the type is first used with RBE. A concept that is evaluated
  earlier caches its answer, and the compiler then rejects the later specialization.

## Using it

```cpp
--8<-- "example/howto/custom.cpp:use"
```

A custom member is decoded as a whole: a lazy view hands it back through `field<"...">().value()`, there are
no fields of its own to read.

!!! warning "Experimental"
    The customization point is implemented and tested, but its shape may change: in particular the wire
    size of a custom type is not yet asked of the specialization. See the
    [feature matrix](../../reference/features.md).

See also: [Core reference, custom](../../reference/core.md#custom-types).
