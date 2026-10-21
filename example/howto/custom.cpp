/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file custom.cpp
 * @brief How-to: encode a type RBE cannot introspect (docs/examples/howto/custom.md)
 */

#include <rbe/rbe.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

// clang-format off
// --8<-- [start:type]
// Not an aggregate: private state, invariants, a constructor. Reflection will not enumerate it for RBE.
class Timestamp {
public:
  constexpr Timestamp() = default;
  constexpr explicit Timestamp(std::chrono::nanoseconds const since_epoch) : ns_(since_epoch.count()) { }

  [[nodiscard]] constexpr auto since_epoch() const -> std::chrono::nanoseconds { return std::chrono::nanoseconds {ns_}; }

  constexpr auto operator==(Timestamp const&) const -> bool = default;

private:
  std::int64_t ns_ {};
};
// --8<-- [end:type]

// --8<-- [start:custom]
// The specialization is the whole opt-in: RBE finds it by its existence. Declare it before the first use
// of the type with RBE, a concept evaluated earlier would have cached the answer without it.
template<>
struct rbe::custom<Timestamp> {
  // write the value at the start of `out`, return the bytes written
  static constexpr auto serialize(std::span<std::byte> const out, Timestamp const& value) -> std::size_t {
    rbe::endian::store<std::int64_t, std::endian::big>(out.data(), value.since_epoch().count());
    return sizeof(std::int64_t);
  }

  // read the value from the start of `in`
  static constexpr auto deserialize(std::span<std::byte const> const in) -> Timestamp {
    return Timestamp {std::chrono::nanoseconds {rbe::endian::load<std::int64_t, std::endian::big>(in.data())}};
  }
};

static_assert(rbe::custom_wirable<Timestamp>);
static_assert(rbe::wire_size_of<Timestamp>() == sizeof(Timestamp));   // the slot on the wire is sizeof(T)
// --8<-- [end:custom]

// --8<-- [start:use]
// Then it is a member like any other
struct [[=rbe::pack_be]] Trade {
  std::uint32_t trade_id;
  Timestamp     executed_at;
  std::uint64_t price;
};
// --8<-- [end:use]
// clang-format on

auto main() -> int {
  using namespace std::chrono_literals;

  std::array<std::byte, rbe::wire_size_of<Trade>()> bytes {};
  auto const trade = Trade {.trade_id = 1, .executed_at = Timestamp {1'700'000'000s}, .price = 2500};

  rbe::serialize(bytes, trade);
  auto const eager = rbe::deserialize<Trade>(bytes, rbe::dsrl::eager);
  auto const lazy  = rbe::deserialize<Trade>(bytes, rbe::dsrl::lazy);

  return eager.executed_at == trade.executed_at and lazy.field<"executed_at">().value() == trade.executed_at ? 0 : 1;
}
