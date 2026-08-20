/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file static_array.hpp
 * @date 07/07/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <initializer_list>
#include <meta>
#include <ranges>
#include <span>

// --- System ---

namespace rbe {

namespace detail {

// TODO: move from public API to detail namespace
template<typename R, typename T>
concept compatible_range = std::ranges::input_range<R> and std::convertible_to<std::ranges::range_reference_t<R>, T>;

} // namespace detail


template<typename T>
  requires std::same_as<std::remove_cvref_t<T>, T>
class static_array {
public:
  using underlying_span_type   = std::span<T const>;
  using element_type           = underlying_span_type::element_type;
  using value_type             = underlying_span_type::value_type;
  using size_type              = underlying_span_type::size_type;
  using difference_type        = underlying_span_type::difference_type;
  using const_pointer          = underlying_span_type::const_pointer;
  using const_reference        = underlying_span_type::const_reference;
  using const_iterator         = underlying_span_type::const_iterator;
  using const_reverse_iterator = underlying_span_type::const_reverse_iterator;

  consteval static_array() = default;

  template<detail::compatible_range<T> R>
  consteval static_array(std::from_range_t /**/, R&& range) :
    data_(std::define_static_array(std::forward<R>(range))) { }

  template<std::input_iterator InputIt>
  consteval static_array(InputIt first, InputIt last) : data_(std::define_static_array(first, last)) { }

  consteval static_array(std::initializer_list<T> init) : data_(std::define_static_array(init)) { }

  [[nodiscard]] constexpr const_iterator begin() const { return data_.begin(); }

  [[nodiscard]] constexpr const_iterator end() const { return data_.end(); }

  [[nodiscard]] constexpr const_reverse_iterator rbegin() const { return data_.rbegin(); }

  [[nodiscard]] constexpr const_reverse_iterator rend() const { return data_.rend(); }

  [[nodiscard]] constexpr const_iterator cbegin() const { return data_.cbegin(); }

  [[nodiscard]] constexpr const_iterator cend() const { return data_.cend(); }

  [[nodiscard]] constexpr const_reverse_iterator crbegin() const { return data_.crbegin(); }

  [[nodiscard]] constexpr const_reverse_iterator crend() const { return data_.crend(); }

  [[nodiscard]] constexpr size_type size() const { return data_.size(); }

  [[nodiscard]] constexpr size_type size_bytes() const { return data_.size_bytes(); }

  [[nodiscard]] constexpr bool empty() const { return data_.empty(); }

  [[nodiscard]] constexpr const_reference operator[](std::size_t index) const { return data_[index]; }

  [[nodiscard]] constexpr const_reference at(std::size_t index) const { return data_.at(index); }

  [[nodiscard]] constexpr const_reference front() const { return data_.front(); }

  [[nodiscard]] constexpr const_reference back() const { return data_.back(); }

  [[nodiscard]] constexpr const_pointer data() const { return data_.data(); }

  [[nodiscard]] consteval static_array first(size_type count) const {
    return {std::from_range, std::define_static_array(data_.first(count))};
  }

  [[nodiscard]] consteval static_array last(size_type offset) const {
    return {std::from_range, std::define_static_array(data_.last(offset))};
  }

  [[nodiscard]] consteval static_array subspan(size_type offset, size_type count = std::dynamic_extent) const {
    return {std::from_range, std::define_static_array(data_.subspan(offset, count))};
  }

  [[nodiscard]] constexpr bool operator==(static_array const& other) const {
    return std::ranges::equal(data_, other.data_);
  }

private:
  std::span<T const> data_;
};

template<std::ranges::range R>
static_array(std::from_range_t, R&&) -> static_array<std::ranges::range_value_t<R>>;

template<std::input_iterator InputIt>
static_array(InputIt, InputIt) -> static_array<std::iter_value_t<InputIt>>;

template<typename T>
static_array(std::initializer_list<T>) -> static_array<T>;

} // namespace rbe
