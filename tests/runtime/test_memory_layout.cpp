/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_layout.cpp
 * @date 27/06/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---

// --- Dependencies ---
#include <rbe/core/fmt.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/srl/serialize.hpp>
#include "rbe/core/concepts.hpp"

// --- External dependencies ---
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// --- STD ---
#include <vector>

// --- System ---

namespace {

// clang-format off
struct [[=rbe::pack]] PacketHeader {
  std::uint16_t length;
  std::uint8_t count;
  std::uint8_t unit;
  std::uint32_t sequence;
};

struct [[=rbe::little, =rbe::pack]] AddOrder {
  [[=rbe::length]] std::uint8_t length;
  [[=rbe::id]] std::uint8_t message_type;
  std::uint32_t time_offset;
  std::uint32_t order_id;
  std::uint8_t side_indicator;
  std::uint32_t quantity;
  std::uint64_t symbol;
  std::uint32_t price;
};

[[=rbe::little]]
struct ReduceSize {
  [[=rbe::length]] std::uint8_t length;
  [[=rbe::id]] std::uint8_t message_type;
  std::uint32_t time_offset;
  std::uint64_t order_id;
  std::uint32_t cancelled_shares;
};

struct [[=rbe::pack]] EmptyPacked {};

struct NoPack {
  std::uint8_t a;
  std::uint32_t b;
};

struct [[=rbe::pack]] Packed {
  std::uint8_t a;
  std::uint32_t b;
};

struct MixedEndian {
  [[=rbe::little]] std::uint32_t a;
  [[=rbe::big]] std::uint32_t b;
  std::uint32_t c; 
};

struct [[=rbe::pack]] Complex {
  [[=rbe::little]] std::uint16_t a;
  [[=rbe::big]] std::uint32_t b;
  std::uint8_t c;
};
// clang-format on


TEST_CASE("Test packet hdr layout") {
  static constexpr rbe::struct_layout packet_hdr          = rbe::get_wire_layout<PacketHeader>();
  static constexpr rbe::struct_layout packet_hdr_expected = {
    .size    = sizeof(PacketHeader),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {offsetof(PacketHeader, length), 0},
        .size   = 2,
      },
      rbe::member_layout {
        .offset = {offsetof(PacketHeader, count), 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {offsetof(PacketHeader, unit), 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {offsetof(PacketHeader, sequence), 0},
        .size   = 4,
      },
    }
  };

  CHECK(packet_hdr == packet_hdr_expected);
}

TEST_CASE("Test packing vs no packing") {
  static constexpr rbe::struct_layout no_pack_layout = rbe::get_wire_layout<NoPack>();
  static constexpr rbe::struct_layout pack_layout    = rbe::get_wire_layout<Packed>();

  CHECK(no_pack_layout.size == sizeof(NoPack));
  CHECK(pack_layout.size == 5); // 1 + 4

  CHECK(no_pack_layout.members[1].offset.bytes == offsetof(NoPack, b));
  CHECK(pack_layout.members[1].offset.bytes == 1);
}

TEST_CASE("Test endianness") {
  static constexpr rbe::struct_layout mixed_layout = rbe::get_wire_layout<MixedEndian>();

  CHECK(mixed_layout.members[0].endianness == rbe::endian::order::little);
  CHECK(mixed_layout.members[1].endianness == rbe::endian::order::big);
  CHECK(mixed_layout.members[2].endianness == rbe::endian::order::native);
}

TEST_CASE("Test complex packed layout with endianness") {
  static constexpr rbe::struct_layout complex_layout = rbe::get_wire_layout<Complex>();

  CHECK(complex_layout.size == 7); // 2 + 4 + 1
  CHECK(complex_layout.members[0].offset.bytes == 0);
  CHECK(complex_layout.members[0].endianness == rbe::endian::order::little);
  CHECK(complex_layout.members[1].offset.bytes == 2);
  CHECK(complex_layout.members[1].endianness == rbe::endian::order::big);
  CHECK(complex_layout.members[2].offset.bytes == 6);
}


} // namespace
