# Overview

RBE turns a plain C++ struct into a wire format. You describe the bytes once, as the struct itself plus a
few [annotations](../reference/annotations.md), and the library derives the serialization and
deserialization code at compile time through C++26 static reflection. There is no schema file, no code
generator and no runtime cost over the equivalent hand-written code: a struct whose in-memory layout already
matches the wire is copied with one `memcpy`.

This page walks through the three layers of the library on one small protocol: a packet header followed by
a run of messages, each one introduced by its own header. The whole example is `example/docs/overview.cpp`.

## Describe the wire format

An annotation is written as `[[=value]]` on a struct or on one of its members. RBE reads it back through
reflection to decide how the type travels; the struct itself is untouched, `sizeof` and member offsets stay
what the compiler says.

```cpp
--8<-- "example/docs/overview.cpp:types"
```

- `=rbe::pack` and `=rbe::big` set the layout and byte order for every member. A member inherits the
  struct's annotations and may override them with its own.
- `=rbe::pack_be` is a preset for the pair; `rbe::derive<...>` builds your own.
- `=rbe::frame_length` and `=rbe::id` mark the fields a frame reads to find where a message ends and which
  message it is. They say nothing about the bytes themselves.
- `Tick` has no annotations: little-endian and the regular C++ layout are the defaults, and because that is
  also its in-memory layout the type is *trivially wirable*.

## Serialize and deserialize

Serialization is one call. Deserialization asks you to pick a strategy, because what a message costs to
read depends on how much of it you need.

```cpp
--8<-- "example/docs/overview.cpp:serde"
```

| Strategy | Returns | When |
|---|---|---|
| `rbe::dsrl::eager` | `T` | you need the whole value, or the buffer will not outlive it |
| `rbe::dsrl::lazy` | `rbe::dsrl::proxy<T>`, a view | you read a few fields; each is decoded on access, nothing is copied |
| `rbe::dsrl::in_place` | `T const&` | the wire layout is the in-memory layout: the buffer is reinterpreted, nothing is decoded |

The functions assume the buffer is large enough. `rbe::try_serialize` and `rbe::try_deserialize` check it
and return `std::nullopt` instead: a wider contract, though a pointer-based one can never be fully wide.

## Frames

A frame is a header followed by a payload. The payload may be a plain struct, one of several structs chosen
by the id in the header (`rbe::frames::any`), a run of frames each walked by its own length
(`rbe::frames::many`), or another frame. A whole protocol is one type:

```cpp
--8<-- "example/docs/overview.cpp:frame_type"
```

Reading it is a view over the buffer, like the lazy strategy: headers are proxies, the run is a range, and
the `any` dispatches on the id with `match`.

```cpp
--8<-- "example/docs/overview.cpp:frame_read"
```

A `match` callback takes the message by value to have it decoded, or as `rbe::dsrl::proxy<T>` to read it
lazily. The `rbe::unmatched` callback is mandatory: it receives ids the code does not know, and the
candidates no other callback handles.

Writing a frame is done part by part today: the header, then each message, each where the previous one
ended.

```cpp
--8<-- "example/docs/overview.cpp:frame_write"
```

!!! info "Planned"
    A value type generated from the frame (`packet::value_type`) and a writer that appends header and
    messages straight into the buffer (`packet::srl_type`) are designed but not implemented. The
    [feature matrix](../reference/features.md#frames) tracks them.

## What you get

- **Declarative**: the struct is the single source of truth for the wire format; frames compose structs into
  a protocol.
- **Checked at compile time**: conflicting annotations, a field name typo in `field<"...">`, a frame built
  from the wrong parts, a `match` missing its fallback: all compile errors.
- **Zero overhead**: everything is derived at compile time; trivially wirable types become a `memcpy`.
- **Header-only, no dependencies**: GCC 16 and `-freflection` are the whole requirement.

## Next steps

- [Installation](installation.md): Conan, CMake or the single header.
- [How-to examples](../examples/howto/annotations.md): one page per task, from annotating a struct to
  writing a custom frame.
- [Market data feeds](../examples/market-data.md): five real protocols modelled end to end.
- [Reference](../reference/annotations.md): every annotation, function and concept.
