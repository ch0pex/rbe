/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_any.cpp
 * @date 21/09/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---
#include "common_frame.hpp"
#include "test_macros.hpp"

#include <rbe/annotations/id.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/framing/dsrl/any.hpp>
#include <rbe/framing/dsrl/any_unmatched.hpp>

//
#include <optional>

namespace {

using namespace dsrl;

inline constexpr std::array<std::byte, 1500> buffer {
  std::byte {0xCC}, std::byte {0xCC}, std::byte {0xCC}, std::byte {0xCC}
};

constexpr auto identity_accessors() {
  any_test any {0, buffer};

  RBE_CHECK(any.is<msg_1>());
  RBE_CHECK(not any.is<msg_2>());
  RBE_CHECK(not any.is<msg_3>());
  RBE_CHECK(any.known_id());
  RBE_CHECK(any.id() == 0);
  RBE_CHECK(any.as<msg_1>() == msg_1 {.value1 = static_cast<int>(0xCCCCCCCC)});
  RBE_CHECK(any.as<msg_2>() == std::nullopt);
  RBE_CHECK(any.as<msg_3>() == std::nullopt);
  RBE_CHECK(any.length() == sizeof(msg_1));
  RBE_CHECK_EQ(any.data(), buffer.data());

  // known ids truncate the span to the wire size of the candidate type,
  // so the span returned by as_span() is a subspan of the original buffer
  auto subspan = std::span<std::byte const> {buffer}.first(sizeof(msg_1));
  RBE_CHECK(std::ranges::equal(any.as_span(), subspan));
}

constexpr auto unknown_id() {
  any_test any {3, buffer};

  RBE_CHECK_FALSE(any.is<msg_1>());
  RBE_CHECK_FALSE(any.is<msg_2>());
  RBE_CHECK_FALSE(any.is<msg_3>());
  RBE_CHECK_FALSE(any.known_id());
  RBE_CHECK(any.id() == 3);
  RBE_CHECK(any.as<msg_1>() == std::nullopt);
  RBE_CHECK(any.as<msg_2>() == std::nullopt);
  RBE_CHECK(any.as<msg_2>() == std::nullopt);

  // whenever the id is unknown, the length of the frame is the size of the buffer,
  // since we cannot know how many bytes are used by the unknown type
  RBE_CHECK(any.length() == buffer.size());
  RBE_CHECK_EQ(any.data(), buffer.data());
  RBE_CHECK(std::ranges::equal(any.as_span(), buffer));
}

constexpr auto any_with_empty_type() {
  any_test_with_heartbeat any {42, buffer};

  RBE_CHECK_FALSE(any.is<msg_1>());
  RBE_CHECK_FALSE(any.is<msg_2>());
  RBE_CHECK_FALSE(any.is<msg_3>());
  RBE_CHECK(any.is<heartbeat>());
  RBE_CHECK(any.known_id());
  RBE_CHECK(any.id() == 42);
  RBE_CHECK(any.as<msg_1>() == std::nullopt);
  RBE_CHECK(any.as<msg_2>() == std::nullopt);
  RBE_CHECK(any.as<msg_3>() == std::nullopt);
  RBE_CHECK(any.as<heartbeat>() == heartbeat {});

  RBE_CHECK(any.length() == 0);
  RBE_CHECK_EQ(any.data(), buffer.data());
  RBE_CHECK(any.as_span().empty());
}

constexpr auto match_known_id() {
  any_test any {0, buffer};

  any.match(
      [](msg_1 m) { RBE_CHECK(m.value1 == static_cast<int>(0xCCCCCCCC)); },
      [](msg_2 /**/) { RBE_FAIL("Should not match msg_2"); }, //
      [](msg_3 /**/) { RBE_FAIL("Should not match msg_3"); }, //
      [](rbe::unmatched auto /**/) { RBE_FAIL("Should not match unknown id"); }
  );

  // any works with proxy
  any.match(
      [](rbe::dsrl::proxy<msg_1> m) { RBE_CHECK(m.field<"value1">() == static_cast<int>(0xCCCCCCCC)); },
      [](msg_2 /**/) { RBE_FAIL("Should not match msg_2"); }, //
      [](msg_3 /**/) { RBE_FAIL("Should not match msg_3"); }, //
      [](rbe::unmatched auto /**/) { RBE_FAIL("Should not match unknown id"); }
  );

  // any don't require all overloads to be present, but there must be a fallback overload for unhandled types
  any.match(
      [](msg_1 m) { RBE_CHECK(m.value1 == static_cast<int>(0xCCCCCCCC)); },
      [](rbe::dsrl::proxy<msg_2> const& /**/) { RBE_FAIL("Should not match msg_2"); },
      [](rbe::unmatched auto /**/) { RBE_FAIL("Should not match unknown"); }
  );

  // any works with template lambdas, all known types will be called with the first callback
  any.match(
      // template lambdas are supported however they need to be constraited to at least
      // avoid being callable with both proxy and non-proxy types, which would lead to ambiguity
      [](rbe::wirable_class auto msg) {
        if constexpr (std::same_as<decltype(msg), msg_1>)
          RBE_CHECK(msg.value1 == static_cast<int>(0xCCCCCCCC));
      },
      // Be aware that if fallback is not explicitly provided with id parameter
      // id will be deduced to the first callback parameter type,
      [](rbe::unmatched auto /**/) { RBE_FAIL("Should not match unknown id"); } // fallback
  );

  auto handle_msg_1_and_2 = [](msgs_1_and_2 auto msg) {
    if constexpr (std::same_as<decltype(msg), msg_1>) {
      RBE_CHECK(msg.value1 == static_cast<int>(0xCCCCCCCC));
    }
    else if constexpr (std::same_as<decltype(msg), msg_2>) {
      RBE_CHECK(msg.numbers[0] == static_cast<int>(0xCCCCCCCC));
    }
    else {
      RBE_FAIL("Should not match unknown type");
    }
  };

  any.match(handle_msg_1_and_2, [](rbe::unmatched auto /**/) { RBE_FAIL("Should not match unknown id"); });

  // theoretically in a real world case this could lead to UB, here it is safe because we know the buffer is large
  // enough to hold msg_2
  any = {1, buffer};
  any.match(handle_msg_1_and_2, [](rbe::unmatched auto /**/) { RBE_FAIL("Should not match unknown id"); });
}

constexpr auto match_unknown_id() {
  any_test any {3, buffer};

  any.match(
      [](msg_1 const&) { RBE_FAIL("Should not match msg_1"); },
      [](msg_2 const&) { RBE_FAIL("Should not match msg_2"); },
      [](msg_3 const&) { RBE_FAIL("Should not match msg_3"); }, //
      [](rbe::unmatched auto unknown_msg) { RBE_CHECK_FALSE(unknown_msg.known_id); }
  );

  any = {2, buffer};
  any.match(
      [](msg_1 const&) { RBE_FAIL("Should not match msg_1"); },
      [](msg_2 const&) { RBE_FAIL("Should not match msg_2"); }, //
      [](rbe::unmatched auto unhandled) { RBE_CHECK(unhandled.known_id); }
  );

  // any.match([](auto msg) { RBE_FAIL("Should not match known types"); }, []() { RBE_CHECK(true); }););
}

constexpr auto match_with_empty_types() {
  any_test_with_heartbeat any {2, buffer};
  any.match(
      [](msg_1 const&) { RBE_FAIL("Should not match msg_1"); },
      [](msg_2 const&) { RBE_FAIL("Should not match msg_2"); }, //
      [](rbe::unmatched auto unhandled) { RBE_CHECK(unhandled.known_id); }
  );

  any = {42, buffer};
  any.match(
      [](msg_1 const&) { RBE_FAIL("Should not match msg_1"); },
      [](msg_2 const&) { RBE_FAIL("Should not match msg_2"); }, //
      [](rbe::unmatched auto unhandled) {
        RBE_CHECK(unhandled.known_id);
        RBE_CHECK(unhandled.data.empty());
      }
  );

  any = {42, buffer};
  any.match(
      [](msg_1 const&) { RBE_FAIL("Should not match msg_1"); },
      [](msg_2 const&) { RBE_FAIL("Should not match msg_2"); }, //
      [](heartbeat const& m) { RBE_CHECK(rbe::wire_size_of<decltype(m)>() == 0); },
      [](rbe::unmatched auto /**/) { RBE_FAIL("Should not match unknown id"); }
  );
}


TEST_SUITE("dsrl_frame_any") {
  RBE_TEST_CASE("dsrl_frame_any - identity_accessors", identity_accessors);
  RBE_TEST_CASE("dsrl_frame_any - unknown_id", unknown_id);
  RBE_TEST_CASE("dsrl_frame_any - match_known_id", match_known_id);
  RBE_TEST_CASE("dsrl_frame_any - match_known_id", match_unknown_id);
  RBE_TEST_CASE("dsrl_frame_any - any_with_empty_type", any_with_empty_type)
  RBE_TEST_CASE("dsrl_frame_any - match_with_empty_types", match_with_empty_types)
}

} // namespace
