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
// clang-format on


TEST_CASE("Test packet hdr layout") {
  static constexpr rbe::struct_layout packet_hdr          = rbe::get_wire_layout<PacketHeader>();
  static constexpr rbe::struct_layout packet_hdr_expected = {
    .size    = sizeof(PacketHeader),
    .members = std::define_static_array(
        std::vector<rbe::member_layout> {
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
    )
  };

  CHECK(packet_hdr == packet_hdr_expected);
}

} // namespace
