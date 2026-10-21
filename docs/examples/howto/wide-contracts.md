# Handle short buffers

Every `rbe::deserialize` and `rbe::serialize` call assumes the buffer is big enough. The `try_` forms check
and return `std::nullopt` instead of undefined behavior.

!!! info "Narrow and wider contracts"
    `rbe::deserialize` has a narrow contract: some inputs it accepts, a short buffer, have undefined
    behavior. The `try_` forms have a *wider* one, not a wide one: they check what can be checked, the
    size and the alignment, and still assume the span points to live memory. See the
    [deserialization reference](../../reference/deserialization.md#wider-contract).

Source: `example/howto/wide_contracts.cpp`, CMake target `howto_wide_contracts`.

## Reading

```cpp
--8<-- "example/howto/wide_contracts.cpp:try_deserialize"
```

For the in-place strategies the check covers alignment too (`in_place_mut` takes a mutable span and returns
`std::optional<T&>`):

```cpp
--8<-- "example/howto/wide_contracts.cpp:alignment"
```

## Frames

A frame checks its header, then that the payload the header announces fits in what is left.

```cpp
--8<-- "example/howto/wide_contracts.cpp:frame"
```

A run of frames (`rbe::frames::many`) never fails this way: it stops at the first incomplete element and
hands back the rest. See [Walk a run of messages](many.md).

## Writing

```cpp
--8<-- "example/howto/wide_contracts.cpp:try_serialize"
```

!!! note
    The `try_` forms check sizes and alignment. They do not, and cannot, check that the memory is live or
    that the bytes mean what the type says: a lazy view over a buffer that is freed later is still a bug.

See also: [Deserialization reference](../../reference/deserialization.md#wider-contract),
[Serialization reference](../../reference/serialization.md).
