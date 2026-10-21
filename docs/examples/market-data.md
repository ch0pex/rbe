# Market data feeds

Five real exchange protocols modelled with RBE, each one a different way of telling where a message ends.
The protocol headers under `example/markets/` declare the messages and compose them into frames; the
driver `example/markets.cpp` serializes a sample of each feed and decodes it back.

CMake target: `markets`.

The wire helper the driver uses writes the parts of a datagram back to back:

```cpp
--8<-- "example/markets.cpp:wire"
```

## Aquis

Every message carries its total length and the packet header carries a count. The packet is
*count-delimited*: the `many` walks exactly `count` messages, measuring each one, and the packet has no
`length()` of its own, only `rbe::frames::distance`.

```cpp
--8<-- "example/markets.cpp:aquis"
```

??? example "example/markets/aquis.hpp"

    ```cpp
    --8<-- "example/markets/aquis.hpp"
    ```

## Cboe PITCH

A length-prefixed unit header wraps a run of messages, each with a one-byte length and a one-byte type.
Thirty-five candidates in one `any`, dispatched by the type.

??? example "example/markets/cboe.hpp"

    ```cpp
    --8<-- "example/markets/cboe.hpp"
    ```

## Cboe TOP

Fixed-width ASCII lines terminated by a line feed, and no length anywhere. `line_view` is a custom frame
that scans for the terminator; `many<line>` walks the stream line by line and an unknown message type is
skipped rather than stopping the walk. The [custom frame how-to](howto/custom-frame.md) takes it apart.

```cpp
--8<-- "example/markets.cpp:cboe_top"
```

## London Stock Exchange

A unit header with a length and a count, then messages that each carry their length: the packet is a `many`
that can be filtered by id with `rbe::views::with_ids` before any field is read.

```cpp
--8<-- "example/markets.cpp:london"
```

??? example "example/markets/london.hpp"

    ```cpp
    --8<-- "example/markets/london.hpp"
    ```

## Nasdaq

SoupBinTCP wraps exactly one ITCH message and declares its length; the ITCH message has no length of its
own. Two nested frames, read with `flatten()`.

```cpp
--8<-- "example/markets.cpp:nasdaq"
```

??? example "example/markets/nasdaq.hpp"

    ```cpp
    --8<-- "example/markets/nasdaq.hpp"
    ```

## OPRA

The header carries a category and a type and no length: the message is *implicitly delimited*, the
candidate the id selects has the size. `Control` is a header-only message, `=rbe::empty`, zero bytes of
payload.

```cpp
--8<-- "example/markets.cpp:opra"
```

??? example "example/markets/opra.hpp"

    ```cpp
    --8<-- "example/markets/opra.hpp"
    ```

## How the frames classify

The driver states, as `static_assert`s, how every protocol classifies at compile time: which frames are
self-delimited, which need a walk, and that a custom frame composes like a library one.

```cpp title="example/markets.cpp"
--8<-- "example/markets.cpp:34:56"
```
