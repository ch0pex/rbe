# Dispatch on a message id

A header carries an id; the id says which of several messages follows. `rbe::frames::any` holds the
candidates and `match` dispatches to the right one.

Source: `example/howto/any_match.cpp`, CMake target `howto_any_match`.

## Declare the candidates

The header marks the field the id is read from with `=rbe::id`; each candidate declares the id it answers
to with `=rbe::id(value)`. The two must have the same type.

```cpp
--8<-- "example/howto/any_match.cpp:types"
```

## Match

One callback per candidate you handle, plus a fallback for everything else.

```cpp
--8<-- "example/howto/any_match.cpp:match"
```

- Take a candidate **by value** (`Order const&`) to have it decoded before the call, or as
  **`rbe::dsrl::proxy<T>`** to read it field by field. Not both: that is ambiguous and does not compile.
- A candidate marked `=rbe::empty` is always taken by value.
- The **`rbe::unmatched`** callback is mandatory. It receives both ids the code does not know and candidates
  no other callback handles; `known_id` tells them apart, `id` is the value on the wire and `data` the
  bytes.
- Every callback must return the same type.

## Ask instead of matching

```cpp
--8<-- "example/howto/any_match.cpp:query"
```

See also: [Frames reference, any](../../reference/frames.md#any-view).
