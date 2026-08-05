/**
 * @file lazy.cpp
 * @brief Demonstrates lazy deserialization strategy.
 *
 * Lazy deserialization returns a lightweight proxy (dsrl::msg<T>) that reads fields
 * on-demand from the buffer without creating a full copy. Fields are accessed one
 * at a time using the proxy's field<>() methods.
 *
 * Use lazy when you need to:
 *  - Avoid copying large messages when only reading a few fields
 *  - Keep memory usage low (no full object allocation)
 *  - Access fields selectively from high-volume message streams
 */

#include <rbe/core/annotations.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/srl/serialize.hpp>

#include <cstdint>
#include <print>
#include <vector>

// ─────────────────────────────────────────────────────────────────────
// Mock data structure
// ─────────────────────────────────────────────────────────────────────

using namespace rbe;
struct[[= rbe::derive<pack, little>]] Trade {
  [[rbe::id]] std::uint8_t message_type; // identifies message
  [[rbe::length]] std::uint16_t length; // total message size
  std::uint32_t trade_id; // trade identifier
  std::uint32_t buyer_id; // buyer party
  std::uint32_t seller_id; // seller party
  std::uint16_t quantity; // trade quantity
  std::uint32_t price; // trade price
  std::uint64_t timestamp; // nanoseconds
};

// ─────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────

int main() {
  // Create and serialize a trade message
  Trade trade_to_send {
    .message_type = 99,
    .length       = sizeof(Trade),
    .trade_id     = 54321,
    .buyer_id     = 111,
    .seller_id    = 222,
    .quantity     = 500,
    .price        = 25075, // 250.75
    .timestamp    = 1722874740000000000ULL,
  };

  std::vector<std::byte> buffer(sizeof(Trade));
  rbe::serialize(buffer, trade_to_send);

  std::println("Serialized {} bytes", buffer.size());

  // ─────────────────────────────────────────────────────────────────
  // LAZY DESERIALIZATION: On-demand field access
  // ─────────────────────────────────────────────────────────────────
  //
  // deserialize<T>(buffer, rbe::lazy) returns dsrl::msg<T>, a proxy
  // that reads fields on-demand without copying. Use field<Index>()
  // or field<"name">() to access individual fields.
  //
  // Advantages:
  //  - No full object construction (memory efficient)
  //  - Only reads fields you actually access
  //  - Buffer stays untouched (no copy)
  //
  // Note: The proxy keeps a reference to the buffer, so the buffer
  // must remain valid for the proxy's lifetime.
  //

  auto trade_view = rbe::deserialize<Trade>(buffer, rbe::dsrl::lazy);

  std::println("\n=== Lazy Deserialization (On-Demand Field Access) ===");

  // Access fields by name using field<"name">() syntax
  // This is the recommended approach for readability and safety
  std::println("Message type: {}", static_cast<int>(trade_view.field<"message_type">()));
  std::println("Trade ID:     {}", trade_view.field<"trade_id">());
  std::println("Buyer ID:     {}", trade_view.field<"buyer_id">());
  std::println("Seller ID:    {}", trade_view.field<"seller_id">());

  // Only fields we access are deserialized from the buffer
  // If we didn't need quantity, timestamp, etc., they're never read
  std::println("Quantity:     {}", trade_view.field<"quantity">());
  std::println("Price:        {}", trade_view.field<"price">());
  std::println("Timestamp:    {}", trade_view.field<"timestamp">());

  // ─────────────────────────────────────────────────────────────────
  // Comparison: High-volume scenario
  // ─────────────────────────────────────────────────────────────────
  //
  // In a real trading system processing 1M messages/sec, lazy
  // deserialization saves memory and cycles when you only need
  // a few fields from each message.
  //

  std::println("\n=== Efficiency Note ===");
  std::println("With lazy, only the fields you access are decoded.");
  std::println("Buffer remains untouched; no full copy is made.");
  std::println("Perfect for filtering/routing in high-frequency scenarios.");

  return 0;
}
