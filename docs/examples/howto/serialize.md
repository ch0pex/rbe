# Serialize into a buffer

Write a value into a `std::byte` buffer, know how many bytes it took, and compose several values back to
back.

Source: `example/howto/serialize.cpp`, CMake target `howto_serialize`.

## A buffer of the right size

`rbe::wire_size_of<T>()` is a constant expression, so a fixed buffer can be sized exactly.

```cpp
--8<-- "example/howto/serialize.cpp:array"
```

## Any contiguous range of bytes

The call takes a `std::span<std::byte>`; `std::vector`, `std::array` and C arrays convert to it.

```cpp
--8<-- "example/howto/serialize.cpp:vector"
```

## Check the buffer first

`rbe::serialize` assumes the buffer is big enough and has undefined behavior otherwise. `rbe::try_serialize`
checks and reports instead.

```cpp
--8<-- "example/howto/serialize.cpp:try"
```

## Several values back to back

Each call writes at the start of the span it is given and returns how far it got. Advance the span by that
much to write the next part.

```cpp
--8<-- "example/howto/serialize.cpp:compose"
```

This is how a frame is written today: header, then payload. See the
[frames reference](../../reference/frames.md#planned) for the serializer that is planned.

See also: [Serialization reference](../../reference/serialization.md).
