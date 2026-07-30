/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file common.hpp
 * @version 1.0
 * @date 30/07/2026
 * @brief Short description
 *
 * Longer description
 */
#pragma once

#include <rbe/core/annotations.hpp>

struct[[= rbe::fmt]] Empty { };

struct[[= rbe::fmt]] B {
  int m0 = 0;
};
struct[[= rbe::fmt]] X {
  int m1 = 1;
};
struct[[= rbe::fmt]] Y {
  int m2 = 2;
};

class[[= rbe::fmt]] Z : public X, public Y {
  int m3 = 3;
  int m4 = 4;
};

struct[[= rbe::fmt]] Inner {
  int x = 10;
  int y = 20;
};

struct[[= rbe::fmt]] Outer {
  Inner inner {};
  int z = 30;
};

struct[[= rbe::fmt]] Deep {
  Outer outer {};
  int w = 40;
};

struct[[= rbe::fmt]] WithVector {
  std::vector<int> values {1, 2, 3};
  int extra = 42;
};

struct[[= rbe::fmt]] WithArray {
  std::array<int, 3> values {1, 2, 3};
  int extra = 42;
};

struct[[= rbe::fmt]] WithSpan {
  std::span<int> values {};
  int extra = 42;
};

struct[[= rbe::fmt]] EmptyBase { };

struct[[= rbe::fmt]] DerivedFromEmpty : public EmptyBase {
  int m0 = 0;
};

struct[[= rbe::fmt]] Base {
  int m0 = 0;
  int m1 = 1;
  int m2 = 2;
};

struct[[= rbe::fmt]] EmptyDerived : Base { };

struct[[= rbe::fmt]] UnnamedMember {
  int _ = 42;
};

struct[[= rbe::fmt]] Bits {
  unsigned int a : 3;
  unsigned int b : 5;
  unsigned int c : 8;
};

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

  constexpr bool operator==(const ReduceSize&) const = default;
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

struct PaddedStruct {
  int a;
  double b;
  char c;
  bool operator==(PaddedStruct const&) const = default;
};

struct NonPaddedStruct {
  int a;
  int b;
  char c;
  bool operator==(NonPaddedStruct const&) const = default;
};

struct EmptyStruct {
  bool operator==(EmptyStruct const&) const = default;
};

// clang-format on
