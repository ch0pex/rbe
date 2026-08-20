/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_concepts.hpp
 * @date 15/07/2026
 * @brief Test RBE concepts
 */

// --- Includes ---
#include "common_structs.hpp"

// --- Dependencies ---
#include <rbe/core.hpp>

namespace {

template<auto Concept, auto TypeList>
consteval auto test_concept() -> bool {
  template for (constexpr auto type: TypeList) {
    if constexpr (not template[:Concept:]<typename[:type:]>) {
      return false;
    }
  }
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

constexpr std::array custom_wirable = {^^NoAggregateCustomSerder};
static_assert(test_concept<^^rbe::trivially_wirable_primitive, trivially_wirable_primitives>());
static_assert(not test_concept<^^rbe::trivially_wirable_primitive, std::array{^^float, ^^double}>()); // not supported yet

static_assert(test_concept<^^rbe::custom_wirable, std::array{^^NoAggregateCustomSerder}>());
static_assert(not test_concept<^^rbe::custom_wirable, trivially_wirable_primitives>());
static_assert(not test_concept<^^rbe::custom_wirable, std::array{^^std::vector<char>, ^^std::string,}>());

static_assert(test_concept<^^rbe::wirable_primitive, trivially_wirable_primitives>());
static_assert(test_concept<^^rbe::wirable_primitive, custom_wirable>());
static_assert(not test_concept<^^rbe::wirable_primitive, std::array{^^std::string, ^^std::vector<int>}>());

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

static_assert(test_concept<^^rbe::wirable, trivially_wirable_primitives>());
static_assert(test_concept<^^rbe::wirable, wirable_structs>());
static_assert(not test_concept<^^rbe::wirable, no_wirable_structs>());

// --- Trivially wirable ---

constexpr auto trivially_wirable_structs = std::array{
  ^^ReduceSize,  ^^NoPack, ^^NonPaddedStruct2, ^^CommonHeader, ^^MessageWithHeader,
  ^^Message, ^^MessageWithEnum,  
};

constexpr auto non_trivially_wirable_structs = std::array{
^^MessageWithHeaderPack, ^^MessageWithHeaderMemberBe, ^^CommonHeaderPackBe, ^^MessageWithHeaderPackBe
};

static_assert(test_concept<^^rbe::trivially_wirable, trivially_wirable_primitives>());
static_assert(test_concept<^^rbe::trivially_wirable, trivially_wirable_structs>());
static_assert(not test_concept<^^rbe::trivially_wirable, non_trivially_wirable_structs>());
static_assert(not test_concept<^^rbe::trivially_wirable, custom_wirable>());
static_assert(not test_concept<^^rbe::trivially_wirable, wirable_structs>());

// static_assert(test_concept<^^rbe::well_annotated, trivially_wirable_primitives>());
// static_assert(test_concept<^^rbe::well_annotated, non_trivially_wirable_structs>());
// static_assert(test_concept<^^rbe::well_annotated, wirable_structs>());

// clang-format on

} // namespace
