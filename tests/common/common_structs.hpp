/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file common_structs.hpp
 * @version 1.0
 * @date 30/07/2026
 * @brief Shared annotated struct definitions used across the test suite
 */
#pragma once

#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/derive.hpp>
#include <rbe/annotations/detail/base.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/annotations/format.hpp>
#include <rbe/annotations/metadata.hpp>
#include <rbe/core/custom.hpp>

#include <type_traits>

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
  bool operator==(Packed const&) const = default;
};

struct MixedEndian {
  [[=rbe::little]] std::uint32_t a;
  [[=rbe::big]] std::uint32_t b;
  std::uint32_t c;
  bool operator==(MixedEndian const&) const = default;
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
  double c;
  bool operator==(NonPaddedStruct const&) const = default;
};

struct NonPaddedStruct2 {
  int a;
  int b;
  char c;
  bool operator==(NonPaddedStruct2 const&) const = default;
};

struct EmptyStruct {
  bool operator==(EmptyStruct const&) const = default;
};

struct CommonHeader {
  std::uint32_t version;
  std::uint16_t size;
  std::uint16_t type;
  std::uint64_t timestamp;
  bool operator==(CommonHeader const&) const = default;
};

struct MessageWithHeader {
  CommonHeader header;
  std::uint32_t price;
  std::uint32_t volume;
  std::uint32_t orders;
  bool operator==(MessageWithHeader const&) const = default;
};

struct [[=rbe::pack, =rbe::big]] CommonHeaderPackBe { 
  std::uint32_t version;
  std::uint16_t size;
  std::uint16_t type;
  std::uint64_t timestamp;
  bool operator==(CommonHeaderPackBe const&) const = default;
};

struct MessageWithHeaderPackBe {
  CommonHeaderPackBe header;
  std::uint32_t price;
  std::uint32_t volume;
  std::uint32_t orders;
  bool operator==(MessageWithHeaderPackBe const&) const = default;
};

struct [[=rbe::pack]] CommonHeaderPack { 
  std::uint32_t version;
  std::uint16_t size;
  std::uint16_t type;
  std::uint16_t symbol;
  bool operator==(CommonHeaderPack const&) const = default;
};

struct MessageWithHeaderPack {
  CommonHeaderPack header;
  std::uint32_t price;
  std::uint32_t volume;
  std::uint32_t orders;
  bool operator==(MessageWithHeaderPack const&) const = default;
};

struct CommonHeaderMemberBe { 
  [[=rbe::big]] std::uint32_t version;
  std::uint16_t size;
  bool operator==(CommonHeaderMemberBe const&) const = default;
};

struct MessageWithHeaderMemberBe {
  CommonHeaderMemberBe header;
  std::uint32_t price;
  std::uint32_t volume;
  std::uint32_t orders;
  bool operator==(MessageWithHeaderMemberBe const&) const = default;
};


class NoAggregateCustomSerder {
public:
  NoAggregateCustomSerder() = default;

  explicit constexpr NoAggregateCustomSerder(std::span<std::byte const> const data)  {
    std::ranges::copy(data, std::ranges::begin(bytes_));
  }

  [[nodiscard]] std::span<std::byte const> bytes() const { return bytes_; }

  bool operator==(NoAggregateCustomSerder const& other) const {
    return bytes_ == other.bytes_;
  };
private:
  std::array<std::byte, 4> bytes_{};
};

template<>
struct rbe::custom<NoAggregateCustomSerder> {
  static constexpr std::size_t serialize(std::span<std::byte> const dst, NoAggregateCustomSerder const& msg) {
    const auto data = msg.bytes();
    std::ranges::copy(msg.bytes(), std::ranges::begin(dst));
    return data.size_bytes();
  }

  static constexpr NoAggregateCustomSerder deserialize(std::span<std::byte const> const data) { return NoAggregateCustomSerder{data}; }
};


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

public:
  bool operator==(NoAggregate const&) const = default;
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

struct MessageWithCArray {
  CommonHeader header;
  std::uint16_t traderID;
  std::array<char, 16> senderID;
  bool operator==(MessageWithCArray const&) const = default;
};

struct MessageWithArray {
  CommonHeader header;
  std::uint16_t traderID;
  std::array<char, 16> senderID;
  bool operator==(MessageWithArray const&) const = default;
};

struct [[=rbe::pack]] MessageWithArrayBe {
  CommonHeader header;
  std::uint16_t traderID;
  [[=rbe::big]] std::array<std::uint32_t, 16> senderID;
  bool operator==(MessageWithArrayBe const&) const = default;
};


//
struct[[=rbe::little]] TestLittle{};
struct[[=rbe::big]]    TestBig{};
struct[[=rbe::pack]]   TestPack{};
struct[[=rbe::id]]     TestId{};
struct[[=rbe::length]] TestLength{};
struct[[=rbe::debug]]  TestDebug{};

inline constexpr struct {} annotation_a {};
inline constexpr struct {} annotation_b {}; // deliberately NOT specialized -- stays a non-RBE annotation
inline constexpr struct {} annotation_c {};

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(annotation_a)>> { };

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(annotation_c)>> { };
struct [[=annotation_a]] AnnotatedStructA {
  int x;
  double y;
};

struct[[=annotation_a, =annotation_b]] AnnotatedStructB {
  int x;
  double y;
};

struct[[=annotation_a, =annotation_c]] AnnotatedStructC {
  int x;
  double y;
};

struct[[=annotation_a, =annotation_c]] AnnotatedStructD {
  [[=annotation_a]] int x;
  double y;
};

struct[[=rbe::derive<annotation_a, annotation_c>]] AnnotatedStructWithList {
  [[=annotation_a]] int x;
  double y;
};

[[=annotation_a]] struct WrongAnnotatedStruct {
  int x;
  double y;
};

struct [[=rbe::big]] Child {

};

struct Parent {
  [[=rbe::pack]] Child child;
};

struct[[=rbe::pack, =rbe::pack]] DuplicatedAnnotations {
  int a, b;
};
struct[[=rbe::little, =rbe::big]] ConflictingAnnotations { };
struct BadParent { [[=rbe::little]] Child child; };

// Neither NestedLeaf nor NestedMiddle carry any annotation of their own -- both must inherit
// big-endian transitively from NestedParent, propagated through two levels of unannotated nesting.
struct NestedLeaf {
  std::uint32_t valor;
  constexpr bool operator==(NestedLeaf const&) const = default;
};

struct NestedMiddle {
  NestedLeaf leaf;
  std::uint32_t valor2;
  constexpr bool operator==(NestedMiddle const&) const = default;
};

struct [[=rbe::big]] NestedParent {
  NestedMiddle node;
  std::uint32_t valor3;
  constexpr bool operator==(NestedParent const&) const = default;
};

// clang-format on
