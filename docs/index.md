# Reflection-Based Encoding (RBE)

A header-only C++ library for declarative binary serialization and deserialization via reflection-based annotations. Describe a binary layout once as plain C++ structs; RBE handles the rest.

The library source code is available on [GitHub](https://github.com/ch0pex/rbe) and is distributed under the [MIT License](https://github.com/ch0pex/rbe/blob/main/LICENSE).

!!! warning "Experimental"
    RBE is at an early, experimental stage. The API is unstable and may change without notice.
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
    - [Quick start](getting-started/quick-start.md): build your first layout step by step
    - [Installation](getting-started/installation.md): compiler requirements, CMake and Conan
    - [User's Guide](guide/annotations.md): annotations, serde and frames in depth
    - [Examples](examples/annotations.md): complete programs, from annotations to real market data feeds
    - [Feature matrix](reference/features.md): what is implemented today
