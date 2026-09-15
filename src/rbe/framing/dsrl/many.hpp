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

// --- STD ---
namespace rbe::dsrl {

template<is_frame T>
class many {
public:
  // --- Type traits ---

  using buffer_type = std::span<std::byte const>;
  using size_type   = std::size_t;
  using frame_type  = T;

  // --- Constructors ---

  constexpr explicit many(buffer_type const data) : data_(data) { }

  constexpr auto next() {
    // assert(not done());
    data_   = data_.subspan(current_->length());
    current = parse_next(data_);
  }

  [[nodiscard]] constexpr auto current() const -> frame_type { return *current_; };

  [[nodiscard]] constexpr auto remainder() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return data_.data(); }

  [[nodiscard]] constexpr auto has_seen_partial() const -> bool { return done() and not remainder().empty(); }

  [[nodiscard]] constexpr auto done() const -> bool { return current_.has_value(); }

private:
  buffer_type data_ {};
  std::optional<frame_type> current_ {};
};


[[nodiscard]] auto parse_next(buffer_type const data) -> std::optional<frame_type> {
  // NOTE: on implictly delimited frames, whose payload is any
  // gathering the length requires dispatching so this is expensive
  // and repeated when calling current(). and repeated calling frame.match
  auto const length = frame_type::length_of(data_);
  if (not length or length > data_.size()) {
    return std::nullopt;
  }
  return frame_type {data_};
}

} // namespace rbe::dsrl
