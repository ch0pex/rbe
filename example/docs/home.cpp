/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file home.cpp
 * @brief Minimal examples included by docs/index.md, one per tab
 */

// --8<-- [start:include_annotations]
#include <rbe/annotations.hpp>
// --8<-- [end:include_annotations]
// --8<-- [start:include_srl]
#include <rbe/srl.hpp>
// --8<-- [end:include_srl]
// --8<-- [start:include_dsrl]
#include <rbe/dsrl.hpp>
// --8<-- [end:include_dsrl]
// --8<-- [start:include_frames]
#include <rbe/frames.hpp>
// --8<-- [end:include_frames]

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace serialize_example {

// clang-format off
// --8<-- [start:serialize_type]
struct Position { std::int32_t x, y, z; };
enum class Item : std::uint16_t { Sword = 1, Shield = 2, Potion = 3 };

// nested types inherit the annotations: Position is packed little-endian too
struct [[=rbe::pack_le]] SaveGame {
    std::uint32_t                version;
    std::uint16_t                level;
    std::uint32_t                health;
    Position                     position;
    std::array<Item, 8>          inventory;
};
// --8<-- [end:serialize_type]
// clang-format on

// docs/index.md quotes these numbers: the annotations change the wire layout, not the struct
static_assert(sizeof(SaveGame) == 40);
static_assert(rbe::wire_size_of<SaveGame>() == 38);

auto run() -> bool {
  // clang-format off
  // --8<-- [start:serialize]
  std::array<std::byte, rbe::wire_size_of<SaveGame>()> file{};
  auto size = rbe::serialize(file, SaveGame{
      .version   = 2,
      .level     = 3,
      .health    = 100,
      .position  = {.x = 10, .y = -20, .z = 0},
      .inventory = { Item::Sword, Item::Potion },
  }); 
  // then write `file` to disk
  // --8<-- [end:serialize]
  // clang-format on
  return size == file.size() and rbe::deserialize<SaveGame>(file, rbe::dsrl::eager).position.y == -20;
}

} // namespace serialize_example

namespace deserialize_example {

// clang-format off
// --8<-- [start:deserialize_type]
struct Version { std::uint8_t major, minor, patch; };

// header at the start of a firmware image, as a bootloader finds it in flash
struct [[=rbe::pack, =rbe::big]] FirmwareHeader {
    std::uint32_t magic;
    Version       version;
    std::uint32_t image_size;
    std::uint32_t crc32;
};
// --8<-- [end:deserialize_type]
// clang-format on

auto run(std::span<std::byte const> flash) -> bool {
  // clang-format off
  // --8<-- [start:deserialize]
  // `flash` holds the bytes at the start of the firmware partition

  // lazy: a view over the bytes, each field is decoded only when read
  auto header  = rbe::deserialize<FirmwareHeader>(flash, rbe::dsrl::lazy);
  auto magic   = header.field<"magic">();            // big-endian to host order, on access
  auto version = header.field<"version">().value();  // a nested struct, decoded as a whole

  // wide contract: std::nullopt instead of undefined behavior if the partition is too small
  auto checked = rbe::try_deserialize<FirmwareHeader>(flash, rbe::dsrl::lazy);
  // --8<-- [end:deserialize]
  // clang-format on
  return magic == 0xB007'10AD and version.minor == 4 and checked.has_value() and
         not rbe::try_deserialize<FirmwareHeader>(flash.first(flash.size() - 1), rbe::dsrl::lazy).has_value();
}

} // namespace deserialize_example

namespace frame_example {

// clang-format off
// --8<-- [start:frame]
struct [[=rbe::pack_be]] Header {
    [[=rbe::frame_length]] std::uint16_t length;
    [[=rbe::id]]           std::uint8_t  type;
};

struct [[=rbe::pack_be, =rbe::id(std::uint8_t{1})]] Order  { std::uint32_t order_ref; std::uint64_t price; };
struct [[=rbe::pack_be, =rbe::id(std::uint8_t{2})]] Cancel { std::uint32_t order_ref; };

using message = rbe::frames::frame<Header, rbe::frames::any<Order, Cancel>>;
// --8<-- [end:frame]
// clang-format on

auto run(std::span<std::byte const> buffer) -> bool {
  // clang-format off
  // --8<-- [start:match]
  auto msg = rbe::deserialize<message>(buffer);

  msg.payload().match(
      [](Order const&) { /* ... */ },              // decoded eagerly
      [](rbe::dsrl::proxy<Cancel>) { /* ... */ },  // or read lazily
      [](rbe::unmatched auto) { /* unknown type */ }
  );
  // --8<-- [end:match]
  // clang-format on
  return msg.header().field<"type">() == 1;
}

} // namespace frame_example

auto main() -> int {
  using namespace frame_example;
  constexpr auto length = rbe::wire_size_of<Header>() + rbe::wire_size_of<Order>();

  auto buffer = std::array<std::byte, length> {};
  auto out    = std::span<std::byte> {buffer};
  out         = out.subspan(rbe::serialize(out, Header {.length = length, .type = 1}));
  rbe::serialize(out, Order {.order_ref = 7, .price = 2500});

  using deserialize_example::FirmwareHeader;
  auto flash = std::array<std::byte, rbe::wire_size_of<FirmwareHeader>()> {};
  rbe::serialize(
      flash, FirmwareHeader {
               .magic = 0xB007'10AD, .version = {.major = 1, .minor = 4, .patch = 2}, .image_size = 4096, .crc32 = 0
             }
  );

  return serialize_example::run() and deserialize_example::run(flash) and frame_example::run(buffer) ? 0 : 1;
}
