/**
 * @file eager.cpp
 * @brief Demonstrates eager deserialization strategy.
 *
 * Eager deserialization fully constructs an object by copying all data from the
 * buffer into a new object. The entire message is materialized in memory at once.
 *
 * Use eager when you need to:
 *  - Own the deserialized data (buffer might be reused/freed)
 *  - Access all or most fields
 *  - Need a lightweight copy into your object
 */

#include <rbe/core/annotations.hpp>
#include <rbe/core/fmt.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/srl/serialize.hpp>

#include <cstdint>
#include <print>
#include <vector>

// ─────────────────────────────────────────────────────────────────────
// Mock data structure
// ─────────────────────────────────────────────────────────────────────

using namespace rbe;

struct[[= rbe::derive<pack, little, fmt>]] Order {
  [[rbe::id]] std::uint8_t message_type; // identifies message
  [[rbe::length]] std::uint16_t length; // total message size
  std::uint32_t order_id; // order identifier
  std::uint16_t quantity; // order quantity
  std::uint32_t price; // price (scaled)
};

// ─────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────

int main() {
  // Create and serialize an order
  Order order_to_send {
    .message_type = 42,
    .length       = sizeof(Order),
    .order_id     = 12345,
    .quantity     = 100,
    .price        = 15050, // 150.50
  };

  // Serialize: eager SRL (Serialization and Retrieval Library)
  std::vector<std::byte> buffer(sizeof(Order));
  rbe::serialize(buffer, order_to_send);

  std::println("Serialized {} bytes", buffer.size());

  // ─────────────────────────────────────────────────────────────────
  // EAGER DESERIALIZATION: Full object copy
  // ─────────────────────────────────────────────────────────────────
  //
  // deserialize<T>(buffer, rbe::eager) returns a fully constructed T
  // with all fields copied from the buffer. This is useful when:
  //  - The buffer is temporary or will be reused
  //  - You're processing many messages and want independent objects
  //  - You need to own the data
  //

  auto [[maybe_unused]] order_received = rbe::deserialize<Order>(buffer, rbe::dsrl::eager);

  std::println("\n=== Eager Deserialization ===");
  // std::println("{}", order_received);

  // The object is now independent of the buffer
  // Even if buffer is cleared, order_received still has valid data
  buffer.clear();

  std::println("\nBuffer cleared, but order_received still accessible:");
  std::println("Order ID:     {}", order_received.order_id);

  return 0;
}
