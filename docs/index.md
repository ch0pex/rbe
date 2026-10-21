# Reflection-Based Encoding (RBE)

A header-only C++ library for declarative binary serialization and deserialization via reflection-based annotations. Describe a binary layout once as plain C++ structs; RBE handles the rest.

The library source code is available on [GitHub](https://github.com/ch0pex/rbe) and is distributed under the [MIT License](https://github.com/ch0pex/rbe/blob/main/LICENSE).

!!! warning "Experimental"
    RBE is at an early, experimental stage. Until version 1.0.0 the API is unstable and breaking changes
    may land in any minor release (`0.1 → 0.2`). Patch releases (`0.1.0 → 0.1.1`) stay backwards compatible.
    Check the [feature matrix](reference/features.md) for what is implemented today.

=== "Serialize"

    ```cpp
    --8<-- "example/docs/home.cpp:include_annotations"
    --8<-- "example/docs/home.cpp:include_srl"

    --8<-- "example/docs/home.cpp:serialize_type"

    --8<-- "example/docs/home.cpp:serialize"
    ```

=== "Deserialize"

    ```cpp
    --8<-- "example/docs/home.cpp:include_annotations"
    --8<-- "example/docs/home.cpp:include_dsrl"

    --8<-- "example/docs/home.cpp:deserialize_type"

    --8<-- "example/docs/home.cpp:deserialize"
    ```

=== "Frames"

    ```cpp
    --8<-- "example/docs/home.cpp:include_annotations"
    --8<-- "example/docs/home.cpp:include_frames"

    --8<-- "example/docs/home.cpp:frame"

    --8<-- "example/docs/home.cpp:match"
    ```

!!! info "What is `[[=rbe::pack_le]]`?"
    It is a C++26 **annotation**, not an attribute. Attributes like `[[nodiscard]]` are hints the
    compiler acts on; an annotation, spelled `[[=value]]`, attaches a constant value to a declaration
    that code can read back through C++26 static reflection. RBE reads it to decide how the type travels on
    the wire: here, packed and little-endian.

    The annotation does not change the struct itself. `SaveGame` keeps its usual C++ layout, padding
    included, so `sizeof(SaveGame)` is still 40; only the bytes RBE writes are packed, and
    `rbe::wire_size_of<SaveGame>()` is 38.

!!! tip "Next steps"
    - [Overview](getting-started/overview.md): the whole library on one protocol, annotations to frames
    - [Installation](getting-started/installation.md): compiler requirements, Conan, CMake and the single header
    - [How-to examples](examples/howto/annotations.md): one short, compiled program per task
    - [Market data feeds](examples/market-data.md): five real protocols modelled end to end
    - [Reference](reference/annotations.md) and [feature matrix](reference/features.md): every symbol, and what is implemented today
