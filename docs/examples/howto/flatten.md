# Flatten nested frames

A frame whose payload is another frame nests as deep as the protocol does. `flatten()` hands back every
header on the way down and the innermost payload in one structured binding.

Source: `example/howto/flatten.cpp`, CMake target `howto_flatten`.

```cpp
--8<-- "example/howto/flatten.cpp:types"
```

## Level by level

```cpp
--8<-- "example/howto/flatten.cpp:nested"
```

## All at once

```cpp
--8<-- "example/howto/flatten.cpp:flatten"
```

`flatten` stops at a payload that is not a frame: a struct, an `any`, a `many` or a `blob`. A frame holding
a run therefore flattens to its header and the run.

See also: [Frames reference, frame view](../../reference/frames.md#frame-view).
