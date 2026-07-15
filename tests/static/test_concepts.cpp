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
#include <rbe/core/concepts.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---

namespace {

enum class test_flags : std::uint8_t { };

struct EmptyMessage { };

struct Message {
  std::uint32_t number;
  std::uint32_t number2;
};

struct MessageWithEnum {
  test_flags flags;
  std::uint32_t number;
  std::uint32_t number2;
};

struct NoAggregate {
  std::uint32_t number;
  std::uint32_t number2;

private:
  std::int32_t private_member;
};

struct AggregateWithPtr {
  std::uint32_t number;
  std::uint32_t number2;
  std::uint32_t* number3_ptr;
};

struct AggregateWithRef {
  std::uint32_t number;
  std::uint32_t number2;
  std::uint32_t& number3_ptr;
};

struct AggregateDerived : Message {
  std::uint32_t numbers_derived;
};

} // namespace

static_assert(not rbe::wirable<EmptyMessage>);
static_assert(rbe::wirable<Message>);
static_assert(rbe::wirable<MessageWithEnum>);
static_assert(not rbe::wirable<NoAggregate>);
static_assert(not rbe::wirable<AggregateWithPtr>);
static_assert(not rbe::wirable<AggregateWithRef>);
static_assert(not rbe::wirable<AggregateDerived>);
