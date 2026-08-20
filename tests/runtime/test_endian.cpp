/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_endian.cpp
 * @date 27/06/2026
 * @brief Doctest suite for rbe::endian conversion and load/store helpers
 */

// --- Includes ---

// --- Dependencies ---
#include <rbe/core/endian.hpp>

// --- External dependencies ---
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// --- STD ---
#include <array>
#include <cstdint>

// --- System ---

namespace endian = rbe::endian;
using order      = rbe::endian::order;

namespace {

using B = std::byte;

// Golden byte representations for each integral type.
// Value uses a distinct byte per position to make endian errors visible.
// le_bytes / be_bytes are the expected byte buffers on store / inputs for load.
template<typename T>
struct traits;

template<>
struct traits<std::uint8_t> {
  static constexpr std::uint8_t value    = 0x42U;
  static constexpr std::uint8_t bswapped = 0x42U;
  static constexpr std::array<B, 1> le_bytes {B {0x42}};
  static constexpr std::array<B, 1> be_bytes {B {0x42}};
};

template<>
struct traits<std::int8_t> {
  static constexpr std::int8_t value    = 0x42;
  static constexpr std::int8_t bswapped = 0x42;
  static constexpr std::array<B, 1> le_bytes {B {0x42}};
  static constexpr std::array<B, 1> be_bytes {B {0x42}};
};

template<>
struct traits<std::uint16_t> {
  static constexpr std::uint16_t value    = 0x0102U;
  static constexpr std::uint16_t bswapped = 0x0201U;
  static constexpr std::array<B, 2> le_bytes {B {0x02}, B {0x01}};
  static constexpr std::array<B, 2> be_bytes {B {0x01}, B {0x02}};
};

template<>
struct traits<std::int16_t> {
  static constexpr std::int16_t value    = 0x0102;
  static constexpr std::int16_t bswapped = 0x0201;
  static constexpr std::array<B, 2> le_bytes {B {0x02}, B {0x01}};
  static constexpr std::array<B, 2> be_bytes {B {0x01}, B {0x02}};
};

template<>
struct traits<std::uint32_t> {
  static constexpr std::uint32_t value    = 0x01020304U;
  static constexpr std::uint32_t bswapped = 0x04030201U;
  static constexpr std::array<B, 4> le_bytes {B {0x04}, B {0x03}, B {0x02}, B {0x01}};
  static constexpr std::array<B, 4> be_bytes {B {0x01}, B {0x02}, B {0x03}, B {0x04}};
};

template<>
struct traits<std::int32_t> {
  static constexpr std::int32_t value    = 0x01020304;
  static constexpr std::int32_t bswapped = 0x04030201;
  static constexpr std::array<B, 4> le_bytes {B {0x04}, B {0x03}, B {0x02}, B {0x01}};
  static constexpr std::array<B, 4> be_bytes {B {0x01}, B {0x02}, B {0x03}, B {0x04}};
};

template<>
struct traits<std::uint64_t> {
  static constexpr std::uint64_t value    = 0x0102030405060708ULL;
  static constexpr std::uint64_t bswapped = 0x0807060504030201ULL;
  static constexpr std::array<B, 8> le_bytes {B {0x08}, B {0x07}, B {0x06}, B {0x05},
                                              B {0x04}, B {0x03}, B {0x02}, B {0x01}};
  static constexpr std::array<B, 8> be_bytes {B {0x01}, B {0x02}, B {0x03}, B {0x04},
                                              B {0x05}, B {0x06}, B {0x07}, B {0x08}};
};

template<>
struct traits<std::int64_t> {
  static constexpr std::int64_t value    = 0x0102030405060708LL;
  static constexpr std::int64_t bswapped = 0x0807060504030201LL;
  static constexpr std::array<B, 8> le_bytes {B {0x08}, B {0x07}, B {0x06}, B {0x05},
                                              B {0x04}, B {0x03}, B {0x02}, B {0x01}};
  static constexpr std::array<B, 8> be_bytes {B {0x01}, B {0x02}, B {0x03}, B {0x04},
                                              B {0x05}, B {0x06}, B {0x07}, B {0x08}};
};


// --- Compile-time tests ---

template<typename T>
consteval bool test_to_native_identity() {
  return endian::to_native<order::native>(traits<T>::value) == traits<T>::value;
}

template<typename T>
consteval bool test_to_native_involution() {
  auto v = traits<T>::value;
  return endian::to_native<order::little>(endian::to_native<order::little>(v)) == v and
         endian::to_native<order::big>(endian::to_native<order::big>(v)) == v;
}

template<typename T>
consteval bool test_to_native_byteswaps() {
  auto v = traits<T>::value;
  if constexpr (order::native == order::little) {
    return endian::to_native<order::little>(v) == v and endian::to_native<order::big>(v) == traits<T>::bswapped;
  }
  else {
    return endian::to_native<order::big>(v) == v and endian::to_native<order::little>(v) == traits<T>::bswapped;
  }
}

template<typename T>
consteval bool test_native_to_alias() {
  auto v = traits<T>::value;
  return endian::native_to<order::little>(v) == endian::to_native<order::little>(v) and
         endian::native_to<order::big>(v) == endian::to_native<order::big>(v) and
         endian::native_to<order::native>(v) == endian::to_native<order::native>(v);
}

template<typename T>
consteval bool test_load_le() {
  return endian::load<T, order::little>(traits<T>::le_bytes.data()) == traits<T>::value;
}

template<typename T>
consteval bool test_load_be() {
  return endian::load<T, order::big>(traits<T>::be_bytes.data()) == traits<T>::value;
}

static_assert(test_to_native_identity<std::uint8_t>());
static_assert(test_to_native_identity<std::int8_t>());
static_assert(test_to_native_identity<std::uint16_t>());
static_assert(test_to_native_identity<std::int16_t>());
static_assert(test_to_native_identity<std::uint32_t>());
static_assert(test_to_native_identity<std::int32_t>());
static_assert(test_to_native_identity<std::uint64_t>());
static_assert(test_to_native_identity<std::int64_t>());

static_assert(test_to_native_involution<std::uint8_t>());
static_assert(test_to_native_involution<std::int8_t>());
static_assert(test_to_native_involution<std::uint16_t>());
static_assert(test_to_native_involution<std::int16_t>());
static_assert(test_to_native_involution<std::uint32_t>());
static_assert(test_to_native_involution<std::int32_t>());
static_assert(test_to_native_involution<std::uint64_t>());
static_assert(test_to_native_involution<std::int64_t>());

static_assert(test_to_native_byteswaps<std::uint8_t>());
static_assert(test_to_native_byteswaps<std::int8_t>());
static_assert(test_to_native_byteswaps<std::uint16_t>());
static_assert(test_to_native_byteswaps<std::int16_t>());
static_assert(test_to_native_byteswaps<std::uint32_t>());
static_assert(test_to_native_byteswaps<std::int32_t>());
static_assert(test_to_native_byteswaps<std::uint64_t>());
static_assert(test_to_native_byteswaps<std::int64_t>());

static_assert(test_native_to_alias<std::uint8_t>());
static_assert(test_native_to_alias<std::int8_t>());
static_assert(test_native_to_alias<std::uint16_t>());
static_assert(test_native_to_alias<std::int16_t>());
static_assert(test_native_to_alias<std::uint32_t>());
static_assert(test_native_to_alias<std::int32_t>());
static_assert(test_native_to_alias<std::uint64_t>());
static_assert(test_native_to_alias<std::int64_t>());

static_assert(test_load_le<std::uint8_t>());
static_assert(test_load_le<std::int8_t>());
static_assert(test_load_le<std::uint16_t>());
static_assert(test_load_le<std::int16_t>());
static_assert(test_load_le<std::uint32_t>());
static_assert(test_load_le<std::int32_t>());
static_assert(test_load_le<std::uint64_t>());
static_assert(test_load_le<std::int64_t>());

static_assert(test_load_be<std::uint8_t>());
static_assert(test_load_be<std::int8_t>());
static_assert(test_load_be<std::uint16_t>());
static_assert(test_load_be<std::int16_t>());
static_assert(test_load_be<std::uint32_t>());
static_assert(test_load_be<std::int32_t>());
static_assert(test_load_be<std::uint64_t>());
static_assert(test_load_be<std::int64_t>());


// --- to_native ---

TEST_CASE_TEMPLATE(
    "to_native<native> is identity", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t, std::uint32_t,
    std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  CHECK(endian::to_native<order::native>(tr::value) == tr::value);
}

TEST_CASE_TEMPLATE(
    "to_native is an involution", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t, std::uint32_t,
    std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  CHECK(endian::to_native<order::little>(endian::to_native<order::little>(tr::value)) == tr::value);
  CHECK(endian::to_native<order::big>(endian::to_native<order::big>(tr::value)) == tr::value);
}

TEST_CASE_TEMPLATE(
    "to_native byteswaps for non-native order", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t,
    std::uint32_t, std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  if constexpr (order::native == order::little) {
    CHECK(endian::to_native<order::little>(tr::value) == tr::value);
    CHECK(endian::to_native<order::big>(tr::value) == tr::bswapped);
  }
  else {
    CHECK(endian::to_native<order::big>(tr::value) == tr::value);
    CHECK(endian::to_native<order::little>(tr::value) == tr::bswapped);
  }
}

TEST_CASE_TEMPLATE(
    "native_to is a symmetric alias for to_native", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t,
    std::uint32_t, std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  CHECK(endian::native_to<order::little>(tr::value) == endian::to_native<order::little>(tr::value));
  CHECK(endian::native_to<order::big>(tr::value) == endian::to_native<order::big>(tr::value));
  CHECK(endian::native_to<order::native>(tr::value) == endian::to_native<order::native>(tr::value));
}


// --- store ---

TEST_CASE_TEMPLATE(
    "store writes correct little-endian bytes", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t,
    std::uint32_t, std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  std::array<B, sizeof(T)> buf {};
  endian::store<T, order::little>(buf.data(), tr::value);
  REQUIRE(buf == tr::le_bytes);
}

TEST_CASE_TEMPLATE(
    "store writes correct big-endian bytes", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t, std::uint32_t,
    std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  std::array<B, sizeof(T)> buf {};
  endian::store<T, order::big>(buf.data(), tr::value);
  REQUIRE(buf == tr::be_bytes);
}


// --- load ---

TEST_CASE_TEMPLATE(
    "load reads correct value from little-endian bytes", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t,
    std::uint32_t, std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  REQUIRE(endian::load<T, order::little>(tr::le_bytes.data()) == tr::value);
}

TEST_CASE_TEMPLATE(
    "load reads correct value from big-endian bytes", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t,
    std::uint32_t, std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  REQUIRE(endian::load<T, order::big>(tr::be_bytes.data()) == tr::value);
}


// --- round-trip ---

TEST_CASE_TEMPLATE(
    "load(store(v)) round-trips value", T, std::uint8_t, std::int8_t, std::uint16_t, std::int16_t, std::uint32_t,
    std::int32_t, std::uint64_t, std::int64_t
) {
  using tr = traits<T>;
  std::array<B, sizeof(T)> buf {};

  endian::store<T, order::little>(buf.data(), tr::value);
  REQUIRE(endian::load<T, order::little>(buf.data()) == tr::value);

  endian::store<T, order::big>(buf.data(), tr::value);
  REQUIRE(endian::load<T, order::big>(buf.data()) == tr::value);
}

} // namespace
