/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file split_span.hpp
 * @date 10/10/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <cassert>
#include <span>

// --- STD ---

namespace rbe::detail {

template<typename T, std::size_t Extent = std::dynamic_extent>
class split_span : std::span<T, Extent> {
  using base = std::span<T, Extent>;

public:
  // --- Type traits ---

  using buffer_type = std::span<T, Extent>;
  using left_type   = std::span<T>;
  using right_type  = std::span<T>;
  using typename base::element_type;
  using typename base::size_type;
  using typename base::value_type;

  // --- Constructors ---

  using base::base;

  // --- Member functions ---

  using base::data;
  using base::size;
  using base::size_bytes;

  [[nodiscard]] constexpr auto left() const -> left_type { return this->first(middle()); }

  [[nodiscard]] constexpr auto right() const -> right_type { return this->subspan(middle()); }

  [[nodiscard]] constexpr auto middle() const -> size_type { return middle_; }

  constexpr auto middle(size_type const position) -> void {
    assert(position <= size());
    middle_ = position;
  }

  constexpr auto advance(size_type const step) -> void {
    assert(middle_ + step <= size());
    middle_ += step;
  }

  constexpr auto retreat(size_type const step) -> void {
    assert(step <= middle_);
    middle_ -= step;
  }

  constexpr auto as_span() const -> buffer_type { return buffer_type {data(), size_bytes()}; }

private:
  size_type middle_ {0};
};

template<typename T>
split_span(T*, std::size_t) -> split_span<T>;

template<typename T, std::size_t Extent>
split_span(std::span<T, Extent>, std::size_t) -> split_span<T>;

} // namespace rbe::detail
