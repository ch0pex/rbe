/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file many.hpp
 * @date 11/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <iterator>
#include <rbe/framing/detail/base_tags.hpp>
#include <rbe/framing/dsrl/concepts.hpp>

// --- STD ---

namespace rbe::dsrl {

template<is_frame T>
  requires self_delimiting_frame<T>
class many : public rbe::detail::many_tag {
public:
  using frame_type  = T;
  using buffer_type = std::span<std::byte const>;
  using size_type   = std::size_t;

  class iterator {
  public:
    using parent_type       = many;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept  = std::input_iterator_tag;
    using value_type        = frame_type;

    explicit constexpr iterator(parent_type* parent) : parent_(parent) { }

    [[nodiscard]] constexpr auto operator*() const -> frame_type { return parent_->current(); }

    [[nodiscard]] constexpr auto operator->() const -> frame_type { return parent_->current(); }

    constexpr auto operator++() -> iterator& {
      parent_->next();
      return *this;
    }

    constexpr auto operator++(int) -> void { ++*this; }

    [[nodiscard]] constexpr auto operator==(std::default_sentinel_t /**/) const -> bool { return parent_->done(); }

  private:
    parent_type* parent_;
  };

  constexpr many(buffer_type const data) : current_(frame_type::make(data)), data_(data) { }

  constexpr auto next() {
    assert(not done());
    data_    = data_.subspan(current_->length());
    current_ = frame_type::make(data_);
  }

  [[nodiscard]] constexpr auto current() const -> frame_type {
    assert(not done());
    return current_.value();
  }

  [[nodiscard]] constexpr auto done() const -> bool { return not current_; }

  [[nodiscard]] constexpr auto remainder() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto has_seen_partial() const -> bool { return done() and not data_.empty(); }

  [[nodiscard]] constexpr auto begin() -> iterator { return iterator(this); }

  [[nodiscard]] constexpr auto end() -> std::default_sentinel_t { return std::default_sentinel; }

  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return data_.data(); }

private:
  std::optional<frame_type> current_;
  buffer_type data_;
};

} // namespace rbe::dsrl
