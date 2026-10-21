/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file reference_deserialization.cpp
 * @brief Snippets included by docs/reference/deserialization.md
 */

#include <rbe/rbe.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace example {

template<rbe::trivially_wirable T>
void signatures(std::span<std::byte> buffer) {
  // clang-format off
  // --8<-- [start:signatures]
  T                   value = rbe::deserialize<T>(buffer, rbe::dsrl::eager);          // decode everything now
  rbe::dsrl::proxy<T> view  = rbe::deserialize<T>(buffer, rbe::dsrl::lazy);           // decode on access
  T const&            ref   = rbe::deserialize<T>(buffer, rbe::dsrl::in_place);       // reinterpret the bytes
  T&                  mut   = rbe::deserialize<T>(buffer, rbe::dsrl::in_place_mut);   // ...and write through them
  // --8<-- [end:signatures]
  // clang-format on
  (void)value, (void)view, (void)ref, (void)mut;
}

template<rbe::trivially_wirable T>
void checked_signatures(std::span<std::byte> buffer) {
  // clang-format off
  // --8<-- [start:try_signatures]
  std::optional<T>                   value = rbe::try_deserialize<T>(buffer, rbe::dsrl::eager);
  std::optional<rbe::dsrl::proxy<T>> view  = rbe::try_deserialize<T>(buffer, rbe::dsrl::lazy);
  std::optional<T const&>            ref   = rbe::try_deserialize<T>(buffer, rbe::dsrl::in_place);
  std::optional<T&>                  mut   = rbe::try_deserialize<T>(buffer, rbe::dsrl::in_place_mut);
  // --8<-- [end:try_signatures]
  // clang-format on
  (void)value, (void)view, (void)ref, (void)mut;
}

// clang-format off
struct [[=rbe::pack_le]] Header {
  [[=rbe::id]] std::uint8_t type;
  std::uint16_t             length;

  auto operator==(Header const&) const -> bool = default;
};

struct [[=rbe::pack_le]] Order {
  Header              header;
  std::uint32_t       order_id;
  std::uint32_t       price;
  std::array<char, 4> tag;

  auto operator==(Order const&) const -> bool = default;
};
// clang-format on

void strategies(std::span<std::byte const> buffer) {
  {
    // clang-format off
    // --8<-- [start:eager]
    Order order = rbe::deserialize<Order>(buffer, rbe::dsrl::eager);
    // --8<-- [end:eager]
    // clang-format on
    (void)order;
  }
  {
    // clang-format off
    // --8<-- [start:lazy]
    rbe::dsrl::proxy<Order> view = rbe::deserialize<Order>(buffer, rbe::dsrl::lazy);

    auto price  = view.field<"price">();      // decoded now, by field name (compile-time checked)
    auto second = view.field<1>();            // decoded now, by declaration index
    // --8<-- [end:lazy]
    // clang-format on
    (void)price, (void)second;
  }
  {
    // clang-format off
    // --8<-- [start:in_place]
    struct Tick { std::uint64_t timestamp; std::uint32_t price; };   // trivially wirable

    alignas(Tick) std::array<std::byte, sizeof(Tick)> aligned {};
    Tick const& tick = rbe::deserialize<Tick>(aligned, rbe::dsrl::in_place);
    // --8<-- [end:in_place]
    // clang-format on
    (void)tick;
  }
}

auto wide(std::span<std::byte const> buffer) -> bool {
  // clang-format off
  // --8<-- [start:try_deserialize]
  auto view = rbe::try_deserialize<Order>(buffer, rbe::dsrl::lazy);          // checks buffer.size() >= wire_size_of<Order>()
  auto cut  = rbe::try_deserialize<Order>(buffer.first(3), rbe::dsrl::lazy); // std::nullopt

  if (view) {
    auto price = view->field<"price">();
    static_cast<void>(price);
  }
  // --8<-- [end:try_deserialize]
  // clang-format on
  return view.has_value() and not cut.has_value();
}

auto proxy_api(std::span<std::byte const> buffer) -> bool {
  // clang-format off
  // --8<-- [start:proxy_api]
  // construction
  rbe::dsrl::proxy<Order>                view    = rbe::deserialize<Order>(buffer, rbe::dsrl::lazy);
  std::optional<rbe::dsrl::proxy<Order>> checked = rbe::dsrl::proxy<Order>::make(buffer);   // nullopt if too short
  rbe::dsrl::proxy<Order>                direct {buffer};                                     // precondition: it fits

  // fields
  std::uint32_t price  = view.field<"price">();          // a primitive member: decoded, by value
  auto          header = view.field<"header">();         // a class member: another proxy (its type carries the
  std::uint8_t  type   = header.field<"type">();         //   inherited annotations, so use auto)
  std::uint8_t  same   = view.field<"header", "type">(); // the same, as a path
  std::uint8_t  by_ann = view.field<rbe::id>();          // the member carrying an annotation, searched in depth
  auto          tag    = view.field<"tag">().value();    // a std::array member is a proxy too: value() decodes it
  auto          second = view.field<1>();                // by declaration index

  // the whole value
  Order order = view.value();
  Order again = *view;

  // extents
  std::size_t                size   = view.size();       // rbe::wire_size_of<Order>(), what the value occupies
  std::span<std::byte const> bytes  = view.as_span();    // exactly those bytes
  std::span<std::byte const> handed = view.buffer();     // the buffer the view was built over, possibly longer
  std::byte const*           data   = view.data();

  // comparison, when Order is equality comparable
  bool equal = view == order and view == direct;
  // --8<-- [end:proxy_api]
  // clang-format on
  return checked.has_value() and type == same and same == by_ann and second == order.order_id and tag[0] == 'a' and
         again == order and price == order.price and size == bytes.size() and handed.data() == data and equal;
}

template<rbe::dsrl::strategy S>
void tags(S const strategy) {
  // clang-format off
  // --8<-- [start:tags]
  static_assert(rbe::dsrl::strategy<rbe::dsrl::eager_t>);
  static_assert(rbe::dsrl::strategy<rbe::dsrl::lazy_t>);
  static_assert(rbe::dsrl::strategy<rbe::dsrl::in_place_t>);
  static_assert(rbe::dsrl::strategy<rbe::dsrl::in_place_mut_t>);

  // what deserialize<T>(buffer, strategy) returns, for generic code
  using result = rbe::dsrl::return_type<S, Order>;
  // --8<-- [end:tags]
  // clang-format on
  static_assert(std::same_as<result, rbe::dsrl::return_type<S, Order>>);
  static_cast<void>(strategy);
}

} // namespace example

auto main() -> int {
  struct Plain {
    std::uint32_t a;
    std::uint32_t b;
  };
  alignas(Plain) std::array<std::byte, sizeof(Plain)> plain {};
  example::signatures<Plain>(plain);
  example::checked_signatures<Plain>(plain);

  std::array<std::byte, rbe::wire_size_of<example::Order>()> bytes {};
  rbe::serialize(
      bytes,
      example::Order {.header = {.type = 5, .length = 15}, .order_id = 9, .price = 100, .tag = {'a', 'b', 'c', 'd'}}
  );
  example::strategies(bytes);
  example::tags(rbe::dsrl::lazy);
  return example::wide(bytes) and example::proxy_api(bytes) ? 0 : 1;
}
