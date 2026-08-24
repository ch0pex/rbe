/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_concepts.cpp
 * @date 15/07/2026
 * @brief Test RBE concepts
 */

// --- Includes ---
#include "common_structs.hpp"

// --- Dependencies ---
#include <rbe/core/detail/invoke_concept.hpp>
#include <rbe/core/trivially_wirable_concepts.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/core/wirable_primitives.hpp>

// --- STD ---
#include <span>

namespace {

template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
consteval auto test_concept(std::meta::info const concept_rfl, R&& type_list) -> bool {
  return std::ranges::all_of(std::forward<R>(type_list), [concept_rfl](std::meta::info const type) {
    return rbe::detail::invoke_concept(concept_rfl, {type});
  });
}

consteval auto test_not_a_concept(std::meta::info const not_a_concept) -> bool try {
  rbe::detail::invoke_concept(not_a_concept, std::array {^^char});
  return false; // invoke_concept should have thrown before reaching here
}
catch (std::meta::exception const&) {
  return true;
}

// clang-format off

// --- Wire primitives ---
constexpr std::array trivially_wirable_primitives = {
  ^^char,      ^^unsigned char, //
  ^^uint8_t,   ^^uint16_t,      ^^uint32_t, ^^uint64_t, //
  ^^int8_t,    ^^int16_t,       ^^int32_t,  ^^int64_t, //
  ^^test_flags // enum example
};

// --- invoke_concept misuse ---
static_assert(test_not_a_concept(^^int));                  // a type is not a concept
static_assert(test_not_a_concept(^^trivially_wirable_primitives)); // a variable is not a concept
static_assert(test_not_a_concept(^^std::vector));          // a class template is not a concept

constexpr std::array custom_wirable = {^^NoAggregateCustomSerder};
static_assert(test_concept(^^rbe::trivially_wirable_primitive, trivially_wirable_primitives));
static_assert(not test_concept(^^rbe::trivially_wirable_primitive, std::array{^^float, ^^double})); // not supported yet

static_assert(test_concept(^^rbe::custom_wirable, std::array{^^NoAggregateCustomSerder}));
static_assert(not test_concept(^^rbe::custom_wirable, trivially_wirable_primitives));
static_assert(not test_concept(^^rbe::custom_wirable, std::array{^^std::vector<char>, ^^std::string,}));

static_assert(test_concept(^^rbe::wirable_primitive, trivially_wirable_primitives));
static_assert(test_concept(^^rbe::wirable_primitive, custom_wirable));
static_assert(not test_concept(^^rbe::wirable_primitive, std::array{^^std::string, ^^std::vector<int>}));

// --- Wirable
constexpr auto wirable_structs = std::array{
  // ^^PaddedStruct, ^^NonPaddedStruct, (doubles not supported yet)
  ^^PacketHeader, ^^AddOrder, ^^ReduceSize,  ^^NoPack, ^^Packed, ^^MixedEndian, ^^Complex,
  ^^NonPaddedStruct2, ^^CommonHeader, ^^MessageWithHeader, ^^NoAggregateCustomSerder,
  ^^Message, ^^MessageWithEnum
};

constexpr auto no_wirable_structs = std::array{
  ^^PaddedStruct, ^^NonPaddedStruct, // (doubles not supported yet)
  ^^NoAggregate, ^^AggregateWithPtr, ^^AggregateWithRef
};

static_assert(test_concept(^^rbe::wirable, trivially_wirable_primitives));
static_assert(test_concept(^^rbe::wirable, wirable_structs));
static_assert(not test_concept(^^rbe::wirable, no_wirable_structs));

// --- Trivially wirable ---

constexpr auto trivially_wirable_structs = std::array{
  ^^ReduceSize,  ^^NoPack, ^^NonPaddedStruct2, ^^CommonHeader, ^^MessageWithHeader,
  ^^Message, ^^MessageWithEnum,  
};

constexpr auto non_trivially_wirable_structs = std::array{
^^MessageWithHeaderPack, ^^MessageWithHeaderMemberBe, ^^CommonHeaderPackBe, ^^MessageWithHeaderPackBe
};

static_assert(test_concept(^^rbe::trivially_wirable, trivially_wirable_primitives));
static_assert(test_concept(^^rbe::trivially_wirable, trivially_wirable_structs));
static_assert(not test_concept(^^rbe::trivially_wirable, non_trivially_wirable_structs));
static_assert(not test_concept(^^rbe::trivially_wirable, custom_wirable));
static_assert(not test_concept(^^rbe::trivially_wirable, wirable_structs));

// static_assert(test_concept(^^rbe::well_annotated, trivially_wirable_primitives));
// static_assert(test_concept(^^rbe::well_annotated, non_trivially_wirable_structs));
// static_assert(test_concept(^^rbe::well_annotated, wirable_structs));

// clang-format on

} // namespace
