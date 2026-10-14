# Reflection-Based Encoding (RBE)

A header-only C++ library for declarative binary serialization and deserialization via reflection-based annotations. Describe a binary layout once as plain C++ structs; RBE handles the rest.

!!! warning "Experimental"
    RBE is at an early, experimental stage. The API is unstable and may change without notice.
    Check the [feature matrix](reference/features.md) for what is implemented today.

## Define the wire format using annotations

```cpp
--8<-- "example/docs/home.cpp:types"
```

## Serialize and deserialize

`rbe::serialize` and `rbe::deserialize` do the work. Deserialization takes an explicit strategy:
eager, lazy or in-place.

```cpp
--8<-- "example/docs/home.cpp:serde"
```

## Framing

A frame is a header followed by a payload, and a payload can be a frame itself, so a whole layout is one type.

```cpp
--8<-- "example/docs/home.cpp:frame_type"

--8<-- "example/docs/home.cpp:frame_read"
```

!!! note "Stub"
    Still to add to this page:

    - Requirements box: a compiler with C++26 reflection and annotations; link to [Installation](getting-started/installation.md)
    - Where to go next: [Quick start](getting-started/quick-start.md), [User's Guide](guide/annotations.md), [Examples](examples/annotations.md), [Reference](reference/annotations.md)
