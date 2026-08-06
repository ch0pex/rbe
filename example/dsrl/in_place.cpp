/**
 * @file in_place.cpp
 * @brief Demonstrates in-place deserialization strategy.
 *
 * In-place deserialization interprets the buffer directly as an object reference
 * without any copying or construction. Only possible when:
 *  - The wire layout exactly matches the in-memory layout (trivially_wirable)
 *  - The buffer is properly aligned for the type
 *
 * Use in-place when you need:
 *  - Absolute maximum performance (zero-copy)
 *  - Direct access to the buffer as if it were your object
 *  - The most memory-efficient approach possible
 */

#include <rbe/core/annotations.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/srl/serialize.hpp>

#include <cstdint>
#include <print>
#include <vector>

// ─────────────────────────────────────────────────────────────────────
// Mock data structure (must be trivially_wirable)
// ─────────────────────────────────────────────────────────────────────
using namespace rbe;
struct[[= rbe::derive<pack, little, fmt>]] Quote {
  std::uint32_t bid_price; // bid price
  std::uint32_t bid_quantity; // bid size
  std::uint32_t ask_price; // ask price
  std::uint32_t ask_quantity; // ask size
};

// ─────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────

int main() {
  // Create and serialize a quote
  Quote quote_to_send {
    .bid_price    = 10050, // 100.50
    .bid_quantity = 1000,
    .ask_price    = 10075, // 100.75
    .ask_quantity = 500,
  };

  std::vector<std::byte> buffer(sizeof(Quote));
  rbe::serialize(buffer, quote_to_send);

  std::println("Serialized {} bytes", buffer.size());

  // ─────────────────────────────────────────────────────────────────
  // IN-PLACE DESERIALIZATION: Zero-copy, direct reference
  // ─────────────────────────────────────────────────────────────────
  //
  // deserialize<T>(buffer, rbe::in_place) returns T& (or const T&),
  // interpreting the buffer directly as an object. This is the fastest
  // possible deserialization with zero memory overhead.
  //
  // Key points:
  //  - No copy occurs; we get a reference to the buffer data
  //  - Only works if wire layout matches in-memory layout
  //  - Buffer lifetime must be >= reference lifetime
  //  - Buffer must be properly aligned
  //

  // Const reference: safe way to read from buffer
  Quote const& [[maybe_unused]] quote_view = rbe::deserialize<Quote>(buffer, rbe::dsrl::in_place);

  std::println("\n=== In-Place Deserialization (Zero-Copy) ===");
  // std::println("{}", quote_view);

  // ─────────────────────────────────────────────────────────────────
  // Mutable reference variant
  // ─────────────────────────────────────────────────────────────────
  //
  // You can also get a mutable reference by using a non-const span:
  // auto& mutable_quote = rbe::deserialize<Quote>(
  //     std::span<std::byte>(buffer), rbe::in_place
  // );
  //
  // Use this only if you need to modify the buffer in-place.
  // This is inherently unsafe if the buffer is shared.
  //

  return 0;
}
