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
#include <limits>
#include <memory>
#include <ranges>
#include <rbe/framing/detail/base_tags.hpp>
#include <rbe/framing/dsrl/concepts.hpp>
#include <rbe/framing/frame_delimiting_concepts.hpp>
#include <utility>

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
    /// Holds the frame operator-> hands back: current() returns by value, so there is nothing to point
    /// into, and the pointer operator-> yields must target a frame that outlives the expression.
    class arrow_proxy {
    public:
      [[nodiscard]] constexpr auto operator->() const -> frame_type const* { return std::addressof(frame_); }

    private:
      friend class iterator;
      explicit constexpr arrow_proxy(frame_type frame) : frame_(std::move(frame)) { }

      frame_type frame_;
    };

    using parent_type       = many;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept  = std::input_iterator_tag;
    using value_type        = frame_type;
    using difference_type   = std::ptrdiff_t;
    using reference         = frame_type;
    using pointer           = arrow_proxy;

    explicit constexpr iterator(parent_type* parent) : parent_(parent) { }

    [[nodiscard]] constexpr auto operator*() const -> reference {
      assert(parent_->current_.has_value());
      return parent_->current();
    }

    [[nodiscard]] constexpr auto operator->() const -> pointer {
      assert(parent_->current_.has_value());
      return arrow_proxy {parent_->current()};
    }

    constexpr auto operator++() -> iterator& {
      assert(not parent_->done());
      parent_->next();
      return *this;
    }

    constexpr auto operator++(int) -> void { ++*this; }

    [[nodiscard]] constexpr auto operator==(std::default_sentinel_t /**/) const -> bool { return parent_->done(); }

  private:
    parent_type* parent_;
  };

  [[nodiscard]] static constexpr auto
  make(buffer_type const data, size_type count = std::numeric_limits<size_type>::max()) -> std::optional<many> {
    return many {data, count};
  }

  explicit constexpr many(buffer_type const data, size_type count = std::numeric_limits<size_type>::max()) :
    current_(frame_type::make(data)), data_(data), left_(count) { }

  constexpr auto next() {
    assert(not done());
    data_    = data_.subspan(current_->length());
    current_ = frame_type::make(data_);
    --left_;
  }

  [[nodiscard]] constexpr auto current() const -> frame_type {
    assert(not done());
    return current_.value();
  }

  [[nodiscard]] constexpr auto done() const -> bool { return not current_ or left_ == 0; }

  [[nodiscard]] constexpr auto remainder() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto has_seen_partial() const -> bool { return done() and not data_.empty(); }

  [[nodiscard]] constexpr auto begin() -> iterator { return iterator(this); }

  [[nodiscard]] constexpr auto end() -> std::default_sentinel_t { return std::default_sentinel; }

  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return data_.data(); }

private:
  std::optional<frame_type> current_;
  buffer_type data_;
  size_type left_;
};

} // namespace rbe::dsrl
