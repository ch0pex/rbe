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

// --- External dependencies ---
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// --- STD ---
#include "common_structs.hpp"

// --- System ---

namespace {

TEST_CASE("Test packet hdr layout") {
  static constexpr rbe::struct_layout packet_hdr          = rbe::get_wire_layout<PacketHeader>();
  static constexpr rbe::struct_layout packet_hdr_expected = {
    .size    = sizeof(PacketHeader),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(PacketHeader, length), .bits = 0},
        .size   = 2,
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(PacketHeader, count), .bits = 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(PacketHeader, unit), .bits = 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(PacketHeader, sequence), .bits = 0},
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
