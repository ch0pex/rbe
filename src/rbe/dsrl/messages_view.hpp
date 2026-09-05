/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file messages_view.hpp
 * @date 05/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---

// --- STD ---

#include <ranges>
#include "rbe/dsrl/any_msg.hpp"
#include "rbe/dsrl/tags.hpp"
namespace rbe::dsrl::views {

namespace detail {

template<is_msg_list T>
class messages {
public:
  using buffer_type  = std::span<std::byte const>;
  using size_type    = std::size_t;
  using msg_list     = T;
  using any_msg_type = any_msg<msg_list>;

  class iterator {
  public:
  private:
  };

  constexpr explicit messages(buffer_type const buffer) { }

  [[nodiscard]] auto remaining() const -> buffer_type { return data_; }

  [[nodiscard]] auto done() -> bool { }

  [[nodiscard]] iterator begin() { }

  [[nodiscard]] iterator end() { }

private:
  buffer_type data_;
};

} // namespace detail

template<is_msg_list T>
struct messages_fn : std::ranges::range_adaptor_closure<messagess> {

  void operator()() { }
};

inline constexpr messages_fn messages;

} // namespace rbe::dsrl::views
