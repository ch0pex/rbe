/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file length.hpp
 * @date 13/10/2026
 * @brief The extent of a frame in bytes: the O(1) one, the one that may walk, and the axis between them
 *
 * The pair mirrors std::ranges::size and std::ranges::distance, and for the same reason: one of them is a
 * question only some types can answer cheaply, the other is an algorithm every type can answer.
 *
 *   rbe::frames::length(f)     the frame's byte extent, O(1). Exists only for a frame that has length().
 *   rbe::frames::distance(f)   the same number, by walking when it has to. Exists for every frame.
 *   rbe::frames::constant_length<F>   whether the first one is available
 *
 * `length()` is a **member and only a member**: a frame is always a class its author writes, so there is
 * nothing to adapt from the outside (which is what ADL buys std::ranges::size, whose range may be a C
 * array). That makes its presence unambiguous, and the presence is the whole cost axis: a frame carries
 * length() when it can answer in O(1) and carries none when it cannot, so constant_length detects rather
 * than believes. It is std::ranges::sized_range exactly, which only works because the member is optional
 * -- rbe::frames::dsrl::is_frame required it until this header existed, and the detection was a tautology.
 *
 * The detection concepts live in rbe::detail and not in a rbe::frames::detail: such a namespace would
 * shadow rbe::detail for every unqualified rbe::detail:: inside rbe::frames, which is where the rest of the
 * classification reads its machinery from.
 *
 * `distance` is the algorithm, not a second customization point, which is the shape std chose too: if you
 * can compute the extent cheaply you expose length(), and if you cannot, walking is the only
 * implementation left. It is still extensible, unlike ranges::distance, because a frame does not always
 * carry something to walk the way a range carries ++: a frame may supply its own `distance()` member, or a
 * `frame_distance(f)` found by ADL, and the library's own walk -- header plus the elements of the run --
 * is the last resort.
 */

#pragma once

// --- Includes ---
#include <rbe/frames/detail/frame_delimiting.hpp>
#include <rbe/frames/frame_concepts.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <ranges>

// The detection concepts live in rbe::detail and NOT in a rbe::frames::detail: such a namespace shadows
// rbe::detail for every unqualified rbe::detail:: inside rbe::frames, which is where the delimiting
// classification reads its machinery from (see A10 in docs/explanation/frame-delimiting-decisions.md).
namespace rbe::detail {

/// A frame that resolves its extent in O(1) and says so by carrying the member
template<typename T>
concept member_length = requires(T const& frame) {
  { frame.length() } -> std::convertible_to<std::size_t>;
};

/// A frame that resolves its extent in its own way, at its own cost
template<typename T>
concept member_distance = requires(T const& frame) {
  { frame.distance() } -> std::convertible_to<std::size_t>;
};

/// ...or declares that resolution next to its type instead of inside it
template<typename T>
concept adl_distance = requires(T const& frame) {
  { frame_distance(frame) } -> std::convertible_to<std::size_t>; // found by ADL in T's own namespace
};

/// A frame that answers for its own extent, cheaply or not: the three ways frames::distance reaches one
template<typename T>
concept resolves_extent = member_length<T> or member_distance<T> or adl_distance<T>;

/// A vocabulary frame, which declares types and no members at all: its API is the view it lowers to
template<typename T>
concept lowers_to_view = requires { typename T::dsrl_type; };

/// A frame whose extent is its header plus the elements of its payload, measured one by one
template<typename T>
concept walkable_run = requires(T const& frame) {
  { frame.header().length() } -> std::convertible_to<std::size_t>;
  requires std::ranges::input_range<decltype(frame.payload())>;
};

} // namespace rbe::detail

namespace rbe::frames {

/**
 * @brief The byte extent of a frame, in O(1)
 *
 * Available only for a frame that carries `length()`, which is the frame stating that it can answer
 * without walking anything -- see `constant_length`. The expensive counterpart is `distance`.
 */
struct length_fn {
  template<is_frame F>
    requires rbe::detail::member_length<F>
  [[nodiscard]] constexpr auto operator()(F const& frame) const -> std::size_t {
    return frame.length();
  }
};

inline constexpr length_fn length {};

/**
 * @brief A frame whose byte extent is available in O(1), the cost axis of the classification
 *
 * The frame answers for itself, by carrying `length()` or by not carrying it -- there is nothing to
 * declare and nothing to opt out of, which is `std::ranges::sized_range` and the reason the axis detects
 * rather than believes.
 *
 * One kind of frame cannot answer, and it is the kind the user writes: the vocabulary
 * `rbe::frames::frame` declares types and no members at all. It delegates to the view it lowers to, which
 * is the type that has the API, and the answer is the same one by construction -- lowering never changes
 * the delimiting category, so it never changes whether `dsrl::frame::length()` exists. Delegating rather
 * than falling back on the deduced walk is what keeps a single answer per frame: a hand-written view that
 * opts out by supplying `distance()` instead of `length()` must not come back as `constant_length` just
 * because the vocabulary type above it has no member to inspect.
 *
 * @tparam F The frame type to be evaluated.
 */
template<typename F>
concept constant_length = //
    is_frame<F> //
    and (rbe::detail::member_length<F> //
         or (not rbe::detail::resolves_extent<F> //
             and rbe::detail::lowers_to_view<F> //
             and rbe::detail::member_length<typename F::dsrl_type>));

/**
 * @brief The byte extent of any frame, walking when that is the only way to it
 *
 * Complexity:
 *  - O(1) only guaranted if the frame carries length()
 *  - O(n) or more depending on adl / member distance implementations provided by the user
 *
 * Chain of calls, in priority order:
 *   - frame.length() -> frame.distance() -> frame_distance(frame) -> header.length() + sum(distance(element) * count)
 */
struct distance_fn {
  template<is_frame F>
    requires rbe::detail::member_length<F> or rbe::detail::member_distance<F> or rbe::detail::adl_distance<F> or
             rbe::detail::walkable_run<F>
  [[nodiscard]] constexpr auto operator()(F const& frame) const -> std::size_t {
    if constexpr (rbe::detail::member_length<F>) {
      return frame.length();
    }
    else if constexpr (rbe::detail::member_distance<F>) {
      return frame.distance();
    }
    else if constexpr (rbe::detail::adl_distance<F>) {
      return frame_distance(frame);
    }
    else {
      // the run bounds the frame and only its elements bound the run: this is the O(n) the cost axis is
      // there to announce, and the reason such a frame carries no length() to be called by accident
      auto run   = frame.payload(); // by value: iterating a run advances it
      auto total = std::size_t {frame.header().length()};
      for (auto const& element: run) {
        total += (*this)(element);
      }
      return total;
    }
  }
};

inline constexpr distance_fn distance {};

} // namespace rbe::frames
