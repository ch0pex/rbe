# Choose a deserialization mode

Read the same bytes three ways and pick the one that matches what you need from them.

Source: `example/howto/deserialization_modes.cpp`, CMake target `howto_deserialization_modes`.

```cpp
--8<-- "example/howto/deserialization_modes.cpp:types"
```

## Eager: the whole value, now

Every field is decoded and the result is an independent object.

```cpp
--8<-- "example/howto/deserialization_modes.cpp:eager"
```

Pick it when you need the whole value, or when the buffer will be reused before you are done with it.

## Lazy: a view, decoded on access

Nothing is read until you ask for a field. The view borrows the buffer, which must stay alive.

```cpp
--8<-- "example/howto/deserialization_modes.cpp:lazy"
```

Pick it to route or filter on a couple of fields out of a large message. See
[Read fields lazily](lazy-fields.md) for everything a view offers.

## In place: the buffer is the object

For a trivially wirable type the wire bytes are the object's bytes: the buffer is reinterpreted, nothing is
decoded and nothing is copied. The buffer must be aligned for the type.

```cpp
--8<-- "example/howto/deserialization_modes.cpp:in_place"
```

Over a mutable buffer, `in_place_mut` returns a mutable reference: writes go straight to the buffer.

```cpp
--8<-- "example/howto/deserialization_modes.cpp:in_place_mut"
```

For a trivially wirable type all three modes end in the same `memcpy`; the choice only matters for types
that need a conversion.

See also: [Deserialization reference](../../reference/deserialization.md).
