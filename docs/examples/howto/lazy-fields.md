# Read fields lazily

Use a `rbe::dsrl::proxy<T>` to read exactly the fields you need, by name, by index, through nested structs,
or by the annotation a field carries.

Source: `example/howto/lazy_fields.cpp`, CMake target `howto_lazy_fields`.

```cpp
--8<-- "example/howto/lazy_fields.cpp:types"
```

## By name or by index

A field name that does not exist is a compile-time error.

```cpp
--8<-- "example/howto/lazy_fields.cpp:access"
```

A member that is a struct, `std::array` included, comes back as another view. `value()` decodes it.

## Nested structs

```cpp
--8<-- "example/howto/lazy_fields.cpp:nested"
```

The type of a nested view carries the annotations it inherits from the enclosing struct, so write `auto`
rather than `rbe::dsrl::proxy<Header>`.

## The whole value

```cpp
--8<-- "example/howto/lazy_fields.cpp:value"
```

See also: [Deserialization reference, proxy](../../reference/deserialization.md#proxy) for the full
interface.
