# Write a custom frame

The frames the library ships find their end by a length field, a count, the id or a fixed size. A protocol
that ends its messages some other way, a line feed here, needs a frame that scans for it. This is the Cboe
TOP feed from the [market data example](../market-data.md): ASCII lines, one message per line, no length
anywhere.

Source: `example/markets/cboe.hpp`, CMake target `markets`.

## The view

A frame view is three steps: read the header, settle the payload's extent, build the payload view. Deriving
from `rbe::frames::dsrl::base_frame` gives the first and third in their default form through the same
building blocks the library uses (`header<H>::make`, `try_construct_payload`), and the derived class writes
the one that differs.

```cpp
--8<-- "example/markets/cboe.hpp:line_view"
```

What the derived class must provide:

- `static make(buffer) -> std::optional<view>`: the wide constructor, which the library calls to walk a run
  and to implement `rbe::try_deserialize`.
- `using delimiting_concept = ...`: which [delimiting category](../../reference/frames.md#delimiting) the
  frame is in. The library cannot deduce it for an extent it does not compute: `self_delimited_tag` says
  "the frame finds its own end".
- `length()`: the frame's extent in bytes, if it can be answered in O(1). It is what lets a `many` advance
  from one line to the next.

Everything else, `header()`, `payload()`, `flatten()`, the spans and the structured binding, comes from
`base_frame`.

## The vocabulary type

The view is what runs; the vocabulary type is what composes. It names the header, the payload, the
category and the view, and that is enough for `rbe::frames::many<line>` and `rbe::deserialize<line>` to
work.

```cpp
--8<-- "example/markets/cboe.hpp:line"
```

## Using it

```cpp
--8<-- "example/markets.cpp:cboe_top"
```

An unknown message type is not an error here: the extent does not depend on the id, so the line is yielded
as an `any` with an unknown id and iteration moves on to the next line.

See also: [Frames reference, custom frames](../../reference/frames.md#custom-frames).
