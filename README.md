# RBE: Reflection Binary Encoding

[![CI](https://github.com/ch0pex/rbe/actions/workflows/ci.yml/badge.svg)](https://github.com/ch0pex/rbe/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/ch0pex/rbe/branch/main/graph/badge.svg)](https://codecov.io/gh/ch0pex/rbe)
[![C++](https://img.shields.io/badge/C%2B%2B-26-blue.svg)](https://en.cppreference.com/w/cpp/26)
[![GCC](https://img.shields.io/badge/GCC-16%2B-blue.svg)](https://gcc.gnu.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Overview

RBE (Reflection Binary Encoding) is a modern, header-only C++26 library that bridges the gap between plain C++ structs and raw binary protocols. By leveraging C++26 static reflection, RBE allows you to declaratively define byte order, packing, and message framing directly via struct attributes.

### Define the wire format using annotations

The library automatically generates highly optimized serialization and deserialization routines at compile time. Whether you are reading from a network socket, a memory-mapped file, or a ring buffer, your struct remains the strict single source of truth, eliminating boilerplate and guaranteeing zero runtime overhead.

```cpp
#include <rbe/annotations.hpp>

enum class msg_type : std::uint8_t { add_order = 0x21, reduce_size = 0x25, tick = 0x2A };

// struct will travel packed and big-endian over the wire
struct [[=rbe::pack, =rbe::big]] PacketHeader {
    std::uint16_t length;
    std::uint8_t  count;
    std::uint8_t  unit;
    std::uint32_t sequence;
};

// =rbe::pack_be is a preset for `=rbe::pack, =rbe::big`
struct [[=rbe::pack_be]] MessageHeader {
    [[=rbe::frame_length]] std::uint8_t length;
    [[=rbe::id]]           msg_type   message_type;
};

struct [[=rbe::pack_be, =rbe::id(msg_type::add_order)]] AddOrder {
    std::uint32_t time_offset;
    std::uint32_t order_id;
    std::uint8_t  side_indicator;
    std::uint32_t quantity;
    std::uint64_t symbol;
    std::uint32_t price;
};

struct [[=rbe::pack_be, =rbe::id(msg_type::reduce_size)]] ReduceSize {
    std::uint32_t time_offset;
    std::uint64_t order_id;
    std::uint32_t cancelled_shares;
};

// trivially_wirable example, wire and host representation are identical
struct [[=rbe::id(msg_type::tick)]] Tick {
    std::uint64_t timestamp;
    std::uint32_t price;
    std::uint32_t quantity;
};

```

### Serialize and deserialize

Serialization and deserialization are as easy as calling `rbe::serialize` and `rbe::deserialize`. The library supports different decoding strategies: eager, lazy and in-place to suit different use cases.

- Eager: decode every field up front, get the struct back
- Lazy: get a proxy over the buffer, each field decoded on access
- In-place: type pun the buffer into a struct, no decoding at all (the type must be `trivially_wirable` and the buffer must be aligned)

```cpp
#include <rbe/dsrl.hpp> // deserialization module
#include <rbe/srl.hpp> // serialization module

alignas(Tick) std::array<std::byte, 1500> buffer{};

// Serialize to raw buffer
auto size = rbe::serialize(buffer, AddOrder{});

// Eager: decode every field up front, get the struct back
auto order = rbe::deserialize<AddOrder>(buffer, rbe::dsrl::eager);
auto price = order.price;

// Lazy: get a proxy over the buffer, each field decoded on access
auto view      = rbe::deserialize<AddOrder>(buffer, rbe::dsrl::lazy);
auto timestamp = view.field<"time_offset">(); // ✅ correct
// auto invalid    = view.field<"invalid">(); // ❌ compile-time error: no such field

// In-place: type pun the buffer into a struct, no decoding at all.
// The type must be `trivially_wirable` and the buffer must meet alignment requirements.
auto& tick = rbe::deserialize<Tick>(buffer, rbe::dsrl::in_place); 
```

### Framing

RBE provides a flexible and recursive message framing system. By defining a frame as a combination of a header and a payload, where payloads can be frames themselves, an entire protocol can be expressed as one unified type.

- Header: any `wirable` type
- Payload: one of
  - a `wirable` type
  - another `rbe::frame`
  - `rbe::any<T...>`: one of several messages, picked by the header's `=rbe::id` field
  - `rbe::many<T>`: a run of messages, each one walked using its own length field

```cpp
#include <rbe/framing.hpp>

using packet = rbe::frame<
    PacketHeader,                                 // packet header
    rbe::many<                                    // ...followed by a run of
        rbe::frame<
            MessageHeader,                        // message header
            rbe::any<AddOrder, ReduceSize, Tick>  // one of these
        >
    >
>;

// Read it back: the strategy picks the view, just like a plain struct
auto packet_view = rbe::deserialize<packet>(buffer, rbe::dsrl::lazy);
auto sequence    = packet_view.header().field<"sequence">();  

for (auto [hdr, payload] : packet_view.payload()) {
  payload.match( // match supports both eager and lazy dispatch
    [](AddOrder add_order) { /* ... */ },,
    [](ReduceSize reduce_size) { /* ... */ },
    [](rbe::proxy<Tick> tick) { /* ... */}, 
    [](msg_type id) { /* unknown message type, handle error */ }
  );
}

// Write it as one value, value_type is automatically generated from the frame's template parameters ...
rbe::serialize<packet>(buffer, packet::value_type { 
    .header  = packet_header,
    .payload = {{msg_header0, add_order}, {msg_header1, reduce_size}},
});

// ...or message by message, straight into the buffer
packet::srl_type {buffer} 
    .header(packet_header)
    .payload()
        .append(msg_header, add_order)
        .append(msg_header, reduce_size);
```

## Key Features

- **Declarative**
  - Describe your binary format with standard C++ structs
  - Describe your frames with `rbe::frame`, `rbe::any` and `rbe::many`
  - Use C++26 annotations to specify endianness, packing, field order, framing and more
- **Safety**
  - Annotations correctness is verified at compile time
  - Proxy member access is verified at compile time during deserialization
  - Frame composition correctness is verified at compile time
- **Performance**
  - Serialization and deserialization logic is generated at compile time, with zero runtime overhead compared to hand-written code
  - Different decoding strategies, eager, lazy and in-place, for different use cases
- **Low adoption cost**
  - Header-only
  - No external dependencies

## Documentation

Full documentation lives at **[ch0pex.github.io/rbe](https://ch0pex.github.io/rbe/)**:

| Section | Start here |
|---|---|
| Tutorials | [Getting started](https://ch0pex.github.io/rbe/tutorials/getting-started/) |
| How-to guides | [Index](https://ch0pex.github.io/rbe/how-to/) |
| Reference | [Annotations](https://ch0pex.github.io/rbe/reference/annotations/) · [Serialization](https://ch0pex.github.io/rbe/reference/serialization/) · [Deserialization](https://ch0pex.github.io/rbe/reference/deserialization/) · [Feature matrix](https://ch0pex.github.io/rbe/reference/features/) |
| Explanation | [Design overview](https://ch0pex.github.io/rbe/explanation/design-overview/) · [Message framing](https://ch0pex.github.io/rbe/explanation/framing/) · [Type concepts](https://ch0pex.github.io/rbe/explanation/concepts/) · [Requirements](https://ch0pex.github.io/rbe/explanation/requirements/) |

## Contributing

Building RBE from source, running the test suite and the repository layout are covered in
[CONTRIBUTING.md](CONTRIBUTING.md).
