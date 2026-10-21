# Walk a run of messages

Messages packed one after the other, each with its own length: `rbe::frames::many` walks them, and the
adaptors in `rbe::views` filter them before a single field is decoded.

Source: `example/howto/many.cpp`, CMake target `howto_many`.

## Declare the run

The element of a `many` must know where it ends on its own: a header with `=rbe::frame_length` or
`=rbe::payload_length`, a candidate set whose id implies the size, or a fixed size. The enclosing header may
bound the run with `=rbe::payload_count`; without it the run goes to the end of the buffer.

```cpp
--8<-- "example/howto/many.cpp:types"
```

## Iterate

A frame decomposes into header and payload with a structured binding, and the payload here is the run.

```cpp
--8<-- "example/howto/many.cpp:iterate"
```

## Filter

```cpp
--8<-- "example/howto/many.cpp:filter"
```

A `many` is a single-pass range: iterating it advances it. `payload()` returns a fresh one each call, so
start every pipeline from the frame rather than from a `many` you have already walked.

## A bare run

With no packet header around the messages, pipe the bytes into a `many` over the message view. Iteration
stops at the first message that is not whole, and `remainder()` is what to keep for the next read.

```cpp
--8<-- "example/howto/many.cpp:stream"
```

See also: [Frames reference, many](../../reference/frames.md#many-view) and
[range adaptors](../../reference/frames.md#range-adaptors).
