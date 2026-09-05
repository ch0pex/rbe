/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_msg_list.cpp
 * @date 03/09/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---
#include "common_structs.hpp"
#include "rbe/core/message_list.hpp"

#include <rbe/annotations/metadata.hpp>
#include <rbe/dsrl/msg.hpp>

// --- STD ---
#include <array>
#include <concepts>
#include <cstdint>
#include <variant>

namespace {

struct NoId {
  std::uint32_t value;
};

struct MockMsg {
  [[= rbe::id]] std::uint16_t id;
};

struct MockMsg2 {
  [[= rbe::id]] std::uint16_t id;
};

struct MockMsg3 {
  [[= rbe::id]] std::uint16_t id;
};

struct DisagreeingId1 {
  [[= rbe::length]] std::uint16_t length;
  [[= rbe::id]] std::uint16_t id;
};

struct DisagreeingId2 {
  [[= rbe::length]] std::uint16_t length;
  std::uint16_t padding;
  [[= rbe::id]] std::uint16_t id;
};

struct DisagreeingLength1 {
  [[= rbe::id]] std::uint16_t id;
  [[= rbe::length]] std::uint16_t length;
};

struct DisagreeingLength2 {
  [[= rbe::id]] std::uint16_t id;
  [[= rbe::length]] std::uint32_t length;
};

struct IdInDifferentPlace {
  std::array<std::byte, 15> padd;
  [[= rbe::id]] std::uint16_t id;
};

struct OrderAdd {
  [[= rbe::length]] std::uint16_t length {sizeof(OrderAdd)};
  [[= rbe::id]] std::uint16_t id {1};
  std::uint32_t price {};
};

struct OrderCancel {
  [[= rbe::length]] std::uint16_t length {sizeof(OrderCancel)};
  [[= rbe::id]] std::uint16_t id {2};
  std::uint64_t order_id {};
};

struct OrderExecute {
  [[= rbe::length]] std::uint16_t length {sizeof(OrderExecute)};
  [[= rbe::id]] std::uint16_t id {3};
  std::uint32_t quantity {};
};

using orders = rbe::msg_list<OrderAdd, OrderCancel, OrderExecute>;

// clang-format off
static_assert(rbe::identificable<MockMsg>);
static_assert(not rbe::identificable<NoId>);
static_assert(not rbe::is_msg_list<rbe::msg_list<MockMsg, MockMsg2, MockMsg3>>);
static_assert(not rbe::is_msg_list<rbe::msg_list<DisagreeingId1, DisagreeingId2>>);
static_assert(not rbe::is_msg_list<rbe::msg_list<DisagreeingLength1, DisagreeingLength2>>);
static_assert(not rbe::is_msg_list<rbe::msg_list<MockMsg, MockMsg2, MockMsg3, IdInDifferentPlace>>);
static_assert(rbe::is_msg_list<orders>);
static_assert(orders::size == 3);
static_assert(std::same_as<orders::id_type, std::uint16_t>);
static_assert(orders::ids == std::array<std::uint16_t, 3> {1, 2, 3});
static_assert(std::same_as<orders::variant_type, std::variant<OrderAdd, OrderCancel, OrderExecute>>);
static_assert(std::same_as<orders::proxy_variant_type, std::variant<rbe::dsrl::msg<OrderAdd>, rbe::dsrl::msg<OrderCancel>, rbe::dsrl::msg<OrderExecute>>>);
// clang-format on

} // namespace
