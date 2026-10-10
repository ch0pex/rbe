/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_split_span.cpp
 * @date 10/10/2026
 * @brief Split span tests
 */

// --- Includes ---
#include "test_macros.hpp"

#include <rbe/core/detail/split_span.hpp>

namespace {

inline constexpr std::array<std::byte, 8> buffer {
  std::byte {0x00}, std::byte {0x01}, std::byte {0x02}, std::byte {0x03}, //
  std::byte {0x04}, std::byte {0x05}, std::byte {0x06}, std::byte {0x07} //
};

constexpr auto test_split_span_middle() {
  rbe::detail::split_span span {buffer};
  span.middle(4);

  RBE_CHECK(span.left().size() == 4);
  RBE_CHECK(span.right().size() == 4);
  RBE_CHECK(span.left()[0] == std::byte {0x00});
  RBE_CHECK(span.right()[0] == std::byte {0x04});

  span.middle(2);
  RBE_CHECK(span.left().size() == 2);
  RBE_CHECK(span.right().size() == 6);
  RBE_CHECK(span.left()[1] == std::byte {0x01});
  RBE_CHECK(span.right()[0] == std::byte {0x02});
}

constexpr auto test_split_span_advance() {
  rbe::detail::split_span span {buffer};
  span.middle(4);

  span.advance(2);
  RBE_CHECK(span.left().size() == 6);
  RBE_CHECK(span.right().size() == 2);
  RBE_CHECK(span.left()[5] == std::byte {0x05});
  RBE_CHECK(span.right()[0] == std::byte {0x06});

  span.advance(2);
  RBE_CHECK(span.left().size() == 8);
  RBE_CHECK(span.right().size() == 0);
  RBE_CHECK(span.left()[7] == std::byte {0x07});
  RBE_CHECK(span.right().empty());
}

constexpr auto test_split_span_retreat() {
  rbe::detail::split_span span {buffer};
  span.middle(6);

  span.retreat(2);
  RBE_CHECK(span.left().size() == 4);
  RBE_CHECK(span.right().size() == 4);
  RBE_CHECK(span.left()[3] == std::byte {0x03});
  RBE_CHECK(span.right()[0] == std::byte {0x04});

  span.retreat(2);
  RBE_CHECK(span.left().size() == 2);
  RBE_CHECK(span.right().size() == 6);
  RBE_CHECK(span.left()[1] == std::byte {0x01});
  RBE_CHECK(span.right()[0] == std::byte {0x02});

  span.retreat(2);
  RBE_CHECK(span.left().size() == 0);
  RBE_CHECK(span.left().empty());
  RBE_CHECK(span.right().size() == 8);
}

} // namespace

TEST_SUITE("split_span") {
  RBE_TEST_CASE("split_span - middle", test_split_span_middle);
  RBE_TEST_CASE("split_span - advance", test_split_span_advance);
  RBE_TEST_CASE("split_span - retreat", test_split_span_retreat);
}
