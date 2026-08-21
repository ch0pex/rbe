/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file common_serde.hpp
 * @version 1.0
 * @date 30/07/2026
 * @brief Shared golden test cases for serialization/deserialization round-trip tests
 */
#pragma once

#include "common_structs.hpp"

#include <rbe/core/wirable_concepts.hpp>

#include <array>

inline constexpr auto pad = std::byte {0xCC};

template<typename... Args>
constexpr std::array<std::byte, sizeof...(Args)> bytes(Args... args) {
  return std::array {static_cast<std::byte>(args)...};
}

template<rbe::wirable T, std::size_t N>
struct TestCase {
  using structure_type = T;
  T structure;
  std::array<std::byte, N> wire;
};

inline constexpr auto ignore_padding = [](std::byte const lhs, std::byte const rhs) {
  return lhs == rhs or lhs == pad or rhs == pad;
};

constexpr TestCase trivially_wirable_no_padding {
  .structure = NonPaddedStruct2 {.a = 1, .b = 2, .c = 'a'},
  .wire      = bytes(
      0x01, 0x00, 0x00, 0x00, // a
      0x02, 0x00, 0x00, 0x00, // b
      0x61, 0x00, 0x00, 0x00 // c
  ),
};

constexpr TestCase trivially_wirable_with_paddings {
  .structure =
      ReduceSize {
        .length           = 0xAA,
        .message_type     = 0xBB,
        .time_offset      = 0x00112233,
        .order_id         = 0x0011223344556677,
        .cancelled_shares = 0x00112233,
      },
  .wire = bytes(
      0xAA, // length
      0xBB, // Msg type
      pad, pad, // padding
      0x33, 0x22, 0x11, 0x00, // time_offset
      0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, // order_id
      0x33, 0x22, 0x11, 0x00, // cancelled shares
      pad, pad, pad, pad // padding
  ),
};

constexpr TestCase wirable_custom_serder {
  .structure = NoAggregateCustomSerder {bytes(0x00, 0x01, 0x02, 0x03)},
  .wire      = bytes(0x00, 0x01, 0x02, 0x03),
};

// Packed structure test case
constexpr TestCase packed_test {
  .structure =
      Packed {
        .a = 0x42,
        .b = 0x12345678,
      },
  .wire = bytes(
      0x42, // a (uint8_t)
      0x78, 0x56, 0x34, 0x12 // b (uint32_t, little-endian by default)
  ),
};

// MixedEndian structure test case
constexpr TestCase mixed_endian_test {
  .structure =
      MixedEndian {
        .a = 0x12345678, // little endian
        .b = 0xAABBCCDD, // big endian
        .c = 0x11223344, // default (little endian)
      },
  .wire = bytes(
      0x78, 0x56, 0x34, 0x12, // a (little-endian)
      0xAA, 0xBB, 0xCC, 0xDD, // b (big-endian)
      0x44, 0x33, 0x22, 0x11 // c (little-endian)
  ),
};

// MessageWithHeader structure test case
constexpr TestCase message_with_header_test {
  .structure =
      MessageWithHeader {
        .header =
            CommonHeader {
              .version   = 1,
              .size      = 100,
              .type      = 5,
              .timestamp = 0x0102030405060708ULL,
            },
        .price  = 99999,
        .volume = 50000,
        .orders = 10,
      },
  .wire = bytes(
      0x01, 0x00, 0x00, 0x00, // version
      0x64, 0x00, // size
      0x05, 0x00, // type
      0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, // timestamp
      0x9F, 0x86, 0x01, 0x00, // price
      0x50, 0xC3, 0x00, 0x00, // volume
      0x0A, 0x00, 0x00, 0x00, // orders
      pad, pad, pad, pad // padding
  ),
};

// CommonHeaderPackBe structure test case
constexpr TestCase common_header_pack_be_test {
  .structure =
      CommonHeaderPackBe {
        .version   = 2,
        .size      = 256,
        .type      = 10,
        .timestamp = 0xAABBCCDDEEFF0011ULL,
      },
  .wire = bytes(
      0x00, 0x00, 0x00, 0x02, // version (big-endian)
      0x01, 0x00, // size (big-endian)
      0x00, 0x0A, // type (big-endian)
      0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11 // timestamp (big-endian)
  ),
};

// MessageWithHeaderPackBe structure test case
constexpr TestCase message_with_header_pack_be_test {
  .structure =
      MessageWithHeaderPackBe {
        .header =
            CommonHeaderPackBe {
              .version   = 1,
              .size      = 50,
              .type      = 3,
              .timestamp = 0x1122334455667788ULL,
            },
        .price  = 12345,
        .volume = 6789,
        .orders = 2,
      },
  .wire = bytes(
      0x00, 0x00, 0x00, 0x01, // version (big-endian)
      0x00, 0x32, // size (big-endian)
      0x00, 0x03, // type (big-endian)
      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, // timestamp (big-endian)
      0x39, 0x30, 0x00, 0x00, // price
      0x85, 0x1A, 0x00, 0x00, // volume
      0x02, 0x00, 0x00, 0x00, // orders
      pad, pad, pad, pad // padding
  ),
};

// CommonHeaderPack structure test case
constexpr TestCase common_header_pack_test {
  .structure =
      CommonHeaderPack {
        .version = 100,
        .size    = 200,
        .type    = 15,
        .symbol  = 42,
      },
  .wire = bytes(
      0x64, 0x00, 0x00, 0x00, // version
      0xC8, 0x00, // size
      0x0F, 0x00, // type
      0x2A, 0x00 // symbol
  ),
};

// MessageWithHeaderPack structure test case
constexpr TestCase message_with_header_pack_test {
  .structure =
      MessageWithHeaderPack {
        .header =
            CommonHeaderPack {
              .version = 5,
              .size    = 32,
              .type    = 8,
              .symbol  = 99,
            },
        .price  = 500,
        .volume = 1000,
        .orders = 3,
      },
  .wire = bytes(
      0x05, 0x00, 0x00, 0x00, // version
      0x20, 0x00, // size
      0x08, 0x00, // type
      0x63, 0x00, // symbol
      pad, pad, 0xF4, 0x01, 0x00, 0x00, // price
      0xE8, 0x03, 0x00, 0x00, // volume
      0x03, 0x00, 0x00, 0x00 // orders
  ),
};

// CommonHeaderMemberBe structure test case
constexpr TestCase common_header_member_be_test {
  .structure =
      CommonHeaderMemberBe {
        .version = 0xDEADBEEF, // big-endian
        .size    = 512, // default (little-endian)
      },
  .wire = bytes(
      0xDE, 0xAD, 0xBE, 0xEF, // version (big-endian)
      0x00, 0x02, // size (little-endian)
      pad, pad // padding
  ),
};

// MessageWithHeaderMemberBe structure test case
constexpr TestCase message_with_header_member_be_test {
  .structure =
      MessageWithHeaderMemberBe {
        .header =
            CommonHeaderMemberBe {
              .version = 0x11223344,
              .size    = 1024,
            },
        .price  = 75000,
        .volume = 25000,
        .orders = 7,
      },
  .wire = bytes(
      0x11, 0x22, 0x33, 0x44, // version (big-endian)
      0x00, 0x04, // size (little-endian)
      pad, pad, // padding
      0xF8, 0x24, 0x01, 0x00, // price
      0xA8, 0x61, 0x00, 0x00, // volume
      0x07, 0x00, 0x00, 0x00 // orders
  ),
};

// NoAggregate structure test case
// constexpr TestCase no_aggregate_test {
//   .structure = NoAggregate {
//     .number = 0xCAFEBABE,
//     .number2 = 0xDEADC0DE,
//   },
//   .wire = bytes(
//       0xBE, 0xBA, 0xFE, 0xCA,           // number
//       0xDE, 0xC0, 0xAD, 0xDE            // number2
//   ),
// };

// MessageWithCArray structure test case
constexpr TestCase message_with_c_array_test {
  .structure =
      MessageWithCArray {
        .header =
            CommonHeader {
              .version   = 1,
              .size      = 50,
              .type      = 2,
              .timestamp = 0x0000000000000042ULL,
            },
        .traderID = 123,
        .senderID = {'T', 'R', 'A', 'D', 'E', 'R', '_', 'I', 'D', '_', '1', '2', '3', '4', '5', '6'},
      },
  .wire = bytes(
      0x01, 0x00, 0x00, 0x00, // version
      0x32, 0x00, // size
      0x02, 0x00, // type
      0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // timestamp
      0x7B, 0x00, // traderID
      0x54, 0x52, 0x41, 0x44, 0x45, 0x52, 0x5F, 0x49, // "TRADER_I"
      0x44, 0x5F, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, // "D_123456"
      pad, pad, pad, pad, pad, pad
  ),
};

// MessageWithArray structure test case
constexpr TestCase message_with_array_test {
  .structure =
      MessageWithArray {
        .header =
            CommonHeader {
              .version   = 2,
              .size      = 75,
              .type      = 4,
              .timestamp = 0x00000000DEADBEEFULL,
            },
        .traderID = 456,
        .senderID = {'S', 'E', 'N', 'D', 'E', 'R', '_', 'I', 'D', '1', '2', '3', '4', '5', '6', '7'},
      },
  .wire = bytes(
      0x02, 0x00, 0x00, 0x00, // version
      0x4B, 0x00, // size
      0x04, 0x00, // type
      0xEF, 0xBE, 0xAD, 0xDE, 0x00, 0x00, 0x00, 0x00, // timestamp
      0xC8, 0x01, // traderID
      0x53, 0x45, 0x4E, 0x44, 0x45, 0x52, 0x5F, 0x49, // "SENDER_I"
      0x44, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, // "D1234567"
      pad, pad, pad, pad, pad, pad
  ),
};

constexpr TestCase message_with_array_be_test {
  .structure =
      MessageWithArrayBe {
        .header =
            CommonHeader {
              .version   = 3,
              .size      = 100,
              .type      = 6,
              .timestamp = 0xEFCDAB9078563412ULL,
            },
        .traderID = 0x0315,
        .senderID =
            {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00, 0x12345678, 0x9ABCDEF0, 0xFEDCBA98, 0x76543210, 0x11111111,
             0x22222222, 0x33333333, 0x44444444, 0x55555555, 0x66666666, 0x77777777, 0x88888888},
      },
  .wire = bytes(
      0x03, 0x00, 0x00, 0x00, // version
      0x64, 0x00, // size
      0x06, 0x00, // type
      0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, // timestamp
      0x15, 0x03, // traderID
      // Array elements in big-endian
      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x12, 0x34, 0x56,
      0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10, 0x11, 0x11, 0x11, 0x11, 0x22, 0x22,
      0x22, 0x22, 0x33, 0x33, 0x33, 0x33, 0x44, 0x44, 0x44, 0x44, 0x55, 0x55, 0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x77,
      0x77, 0x77, 0x77, 0x88, 0x88, 0x88, 0x88
  ),
};

// NestedParent structure test case -- regression for N-level annotation propagation: neither
// NestedMiddle nor NestedLeaf carry an endianness annotation of their own, so both must inherit
// big-endian transitively from NestedParent, two and one levels up respectively, rather than
// silently defaulting to native the moment an intermediate level in the chain has nothing explicit.
constexpr TestCase nested_propagation_test {
  .structure =
      NestedParent {
        .node   = {.leaf = {.valor = 0x11223344}, .valor2 = 0xAABBCCDD},
        .valor3 = 0x55667788,
      },
  .wire = bytes(
      0x11, 0x22, 0x33, 0x44, // node.leaf.valor (big-endian, inherited two levels up)
      0xAA, 0xBB, 0xCC, 0xDD, // node.valor2 (big-endian, inherited one level up)
      0x55, 0x66, 0x77, 0x88 // valor3 (big-endian, its own field)
  ),
};
