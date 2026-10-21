# Print a type for debugging

`=rbe::fmt` gives a struct a `std::formatter` and an `operator<<` that print every member, recursively,
with the member names.

Source: `example/howto/format.cpp`, CMake target `howto_format`.

```cpp
--8<-- "example/howto/format.cpp:types"
```

```cpp
--8<-- "example/howto/format.cpp:print"
```

- A nested struct is printed through its own formatter, so it needs `=rbe::fmt` too (or a `std::formatter`
  of its own). Members of standard types print as `std::format` prints them.
- `rbe::debug` is `rbe::derive<rbe::fmt>`: a preset, so it reads like the wire presets next to it.
- The annotation has no effect on the wire.

See also: [Core reference, formatting](../../reference/core.md#formatting).
