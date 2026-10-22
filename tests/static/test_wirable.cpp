/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_wirable.cpp
 * @date 21/10/2026
 * @brief test wirable concepts
 */

// --- Includes ---
#include "common_structs.hpp"

#include <rbe/core/detail/invoke_concept.hpp>
#include <rbe/core/wirable_concepts2.hpp>

// --- STD ---
#include <algorithm>
#include <array>
#include <inplace_vector>
#include <span>
#include <vector>

namespace {

template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
consteval auto test_concept(std::meta::info const concept_rfl, R&& type_list) -> bool {
  return std::ranges::all_of(std::forward<R>(type_list), [concept_rfl](std::meta::info const type) {
    return rbe::detail::invoke_concept(concept_rfl, {type});
  });
}

template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
consteval auto test_none(std::meta::info const concept_rfl, R&& type_list) -> bool {
  return std::ranges::none_of(std::forward<R>(type_list), [concept_rfl](std::meta::info const type) {
    return rbe::detail::invoke_concept(concept_rfl, {type});
  });
}

// clang-format off
constexpr auto primitives = std::array{^^int, ^^char, ^^uint8_t, ^^int64_t, ^^test_flags};

constexpr auto static_ranges = std::array{
  ^^std::array<int, 5>, 
  ^^std::array<std::array<int, 5>, 5>, 
  ^^std::array<Message, 2>,
};

constexpr auto dynamic_ranges = std::array{
  ^^std::vector<int>, 
  ^^std::vector<std::array<int, 5>>,
  ^^std::inplace_vector<int, 5>
};

constexpr auto classes = std::array{
  ^^PacketHeader, ^^AddOrder, ^^ReduceSize, ^^NoPack, ^^Packed, ^^MixedEndian, ^^Complex, ^^NonPaddedStruct2,
  ^^CommonHeader, ^^MessageWithHeader, ^^Message, ^^MessageWithEnum, ^^FriendlyNoAggregate,
};

constexpr auto not_wirable = std::array{
  ^^float, ^^double, ^^int*, ^^EmptyMessage, ^^AggregateDerived,
  ^^NonPaddedStruct, // (doubles not supported yet)
  ^^NoAggregate, ^^AggregateWithPtr, ^^AggregateWithRef, ^^FriendlyDoesNotOpenMembers,
  ^^std::array<double, 2>, ^^std::vector<float>,
  ^^std::span<uint8_t const>, // non-owning
};

// --- wirable: the root concept accepts every kind ---
static_assert(test_concept(^^rbe::wirable, primitives));
static_assert(test_concept(^^rbe::wirable, static_ranges));
static_assert(test_concept(^^rbe::wirable, dynamic_ranges));
static_assert(test_concept(^^rbe::wirable, classes));
static_assert(test_none(^^rbe::wirable, not_wirable));

// --- wirable_range: only contiguous sized ranges of wirable values ---
static_assert(test_concept(^^rbe::wirable_range, static_ranges));
static_assert(test_concept(^^rbe::wirable_range, dynamic_ranges));
static_assert(test_none(^^rbe::wirable_range, primitives));
static_assert(test_none(^^rbe::wirable_range, classes));
static_assert(test_none(^^rbe::wirable_range, not_wirable));

// --- wirable_static_range: size known at compile time ---
static_assert(test_concept(^^rbe::wirable_static_range, static_ranges));
static_assert(test_none(^^rbe::wirable_static_range, dynamic_ranges));
static_assert(test_none(^^rbe::wirable_static_range, primitives));
static_assert(test_none(^^rbe::wirable_static_range, classes));
static_assert(test_none(^^rbe::wirable_static_range, not_wirable));

// --- wirable_dynamic_range: size known only at runtime ---
static_assert(test_concept(^^rbe::wirable_dynamic_range, dynamic_ranges));
static_assert(test_none(^^rbe::wirable_dynamic_range, static_ranges));
static_assert(test_none(^^rbe::wirable_dynamic_range, primitives));
static_assert(test_none(^^rbe::wirable_dynamic_range, classes));
static_assert(test_none(^^rbe::wirable_dynamic_range, not_wirable));

// --- wirable_class: only classes whose members are all wirable ---
static_assert(test_concept(^^rbe::wirable_class, classes));
static_assert(test_none(^^rbe::wirable_class, primitives));
static_assert(test_none(^^rbe::wirable_class, static_ranges));
static_assert(test_none(^^rbe::wirable_class, dynamic_ranges));
static_assert(test_none(^^rbe::wirable_class, not_wirable));
// clang-format on

} // namespace
