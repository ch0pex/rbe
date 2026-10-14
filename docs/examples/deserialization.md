# Deserialization Modes

The same data read with each strategy. See [Deserialization Modes](../guide/serde/deserialization-modes.md)
for when to pick each one.

=== "Eager"

    CMake target: `eager_deserialization`

    ```cpp title="example/dsrl/eager.cpp"
    --8<-- "example/dsrl/eager.cpp"
    ```

=== "Lazy"

    CMake target: `lazy_deserialization`

    ```cpp title="example/dsrl/lazy.cpp"
    --8<-- "example/dsrl/lazy.cpp"
    ```

=== "In-place"

    CMake target: `in_place`

    ```cpp title="example/dsrl/in_place.cpp"
    --8<-- "example/dsrl/in_place.cpp"
    ```
