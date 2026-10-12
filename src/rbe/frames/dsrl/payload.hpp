/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file payload.hpp
 * @date 06/10/2026
 * @brief The steps a frame takes to reach its payload, as public building blocks
 *
 * Reading a frame is three steps: locate the payload (the header's job, header<H>::make and length()),
 * settle where it ends (payload_extent) and build a view over those bytes (try_construct_payload). dsrl::frame is
 * the canonical composition of the three, and nothing else; they are public so that a frame of the
 * user's own -- one that delimits by a terminator, by a composite discriminant, by a length carried in
 * the body -- composes the same blocks and swaps only the step it needs, instead of copying frame's
 * internals to stay interoperable with any, proxy and nested frames.
 *
 * Each step comes in the two forms the rest of the library uses: a narrow one that assumes its
 * precondition (payload_extent, construct_payload) and a wide one that reports failure as std::optional
 * (try_payload_extent, try_construct_payload).
 */

#pragma once

// --- Includes ---
#include <rbe/dsrl/proxy.hpp>
#include <rbe/frames/dsrl/header.hpp>
#include <rbe/frames/frame_concepts.hpp>
#include <rbe/frames/frame_delimiting_concepts.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <optional>
#include <span>

namespace rbe::frames::dsrl {

namespace detail {

template<frames::payload T>
struct normalize_payload {
  using type = T;
};

// NOTE: `not is_frame<T>` because rbe::wirable only inspects a class's own non-static data members: a frame
// view whose state lives in base_frame has none of its own and passes for wirable. That is a gap in
// detail::is_wirable_class_type (bases are ignored), not a property of frames; the guard keeps it out of here.
template<frames::payload T>
  requires(frames::wirable_payload<T> and not is_frame<T>)
struct normalize_payload<T> {
  using type = rbe::dsrl::proxy<T>;
};

template<typename P>
concept has_make = requires(std::span<std::byte const> const bytes) {
  { P::make(bytes) } -> std::same_as<std::optional<P>>;
};

template<frames::header H>
constexpr auto payload_count(header<H> const hdr) -> std::size_t {
  if constexpr (header<H>::has_count) {
    return hdr.payload_count();
  }
  else {
    return std::numeric_limits<std::size_t>::max();
  }
}

/**
 * @brief A run whose extent the header settles: a count field over elements of a compile-time-constant size
 *
 * The one shape where a count delimits without walking anything, and the same condition
 * rbe::detail::frame_delimiting reads to hand such a frame frames::static_stride_delimited_tag -- the two
 * must agree, since one settles the bytes and the other the category of the frame over them.
 */
template<typename P, typename H>
concept counted_static_run = header<H>::has_count and frames::static_stride<P>;

} // namespace detail

/// The view a frame hands back for a payload declared as P: rbe::dsrl::proxy<T> for a wirable T, P itself otherwise
template<frames::payload P>
using payload_view_t = typename detail::normalize_payload<P>::type;


/// The bytes a count field bounds: the run is `count` elements of a constant size, and no element is read
template<frames::payload P, frames::header H>
  requires detail::counted_static_run<P, H>
[[nodiscard]] constexpr auto counted_extent(header<H> const hdr) -> std::size_t {
  return hdr.payload_count() * frames::static_length<frames::many_element_t<P>>;
}

/**
 * @brief The bytes the payload declared as P occupies at the start of `rest`, the bytes after the header
 *
 * The extent the header declares (payload_length / frame_length) when it declares one, whatever the
 * payload is; then a count over a static-stride run, which the header settles just as well (the only
 * place the payload *type* takes part in step 2, and the runtime half of
 * frames::static_stride_delimited); otherwise `rest`, the payload runs to the end of the buffer and the
 * view built over it in step 3 narrows itself if it can. Narrow contract: `rest` holds the extent the
 * header declares. try_payload_extent is the wide form.
 */
template<frames::payload P, frames::header H>
[[nodiscard]] constexpr auto payload_extent([[maybe_unused]] header<H> const hdr, std::span<std::byte const> const rest)
    -> std::span<std::byte const> {
  if constexpr (header<H>::delimits_payload) {
    return rest.first(hdr.payload_length());
  }
  else if constexpr (detail::counted_static_run<P, H>) {
    return rest.first(counted_extent<P>(hdr));
  }
  else {
    return rest;
  }
}

/// Wide form of payload_extent: nullopt when `rest` does not hold the extent the header declares
template<frames::payload P, frames::header H>
[[nodiscard]] constexpr auto
try_payload_extent([[maybe_unused]] header<H> const hdr, std::span<std::byte const> const rest)
    -> std::optional<std::span<std::byte const>> {
  if constexpr (header<H>::delimits_payload) {
    return rest.size() >= hdr.payload_length() ? std::optional {rest.first(hdr.payload_length())} : std::nullopt;
  }
  else if constexpr (detail::counted_static_run<P, H>) {
    auto const extent = counted_extent<P>(hdr);
    return rest.size() >= extent ? std::optional {rest.first(extent)} : std::nullopt;
  }
  else {
    return rest;
  }
}

/**
 * @brief The view over `bytes`, the payload's extent as already settled by step 2 (or by the caller)
 *
 * The view narrows itself to what it knows: a proxy to T's wire size, an any to its candidate's wire size
 * (the whole of `bytes` for an unknown id), a nested frame to its own length. The bytes a header declared
 * beyond that stay reachable through the frame's payload_span(), not through the view. Narrow contract:
 * the payload fits in `bytes`. try_construct_payload is the wide form.
 */
template<frames::payload P, frames::header H>
[[nodiscard]] constexpr auto
construct_payload([[maybe_unused]] header<H> const hdr, std::span<std::byte const> const bytes) -> payload_view_t<P> {
  using view = payload_view_t<P>;
  if constexpr (frames::is_any<view>) {
    static_assert(
        header<H>::has_id,
        "an any payload selects its candidate by the header's rbe::id field, and this header has none"
    );
    return view {hdr.id(), bytes};
  }
  else if constexpr (frames::is_many<view>) {
    return view {bytes, detail::payload_count(hdr)}; // many
  }
  else {
    return view {bytes};
  }
}

/// Wide form of construct_payload: nullopt when the payload does not fit in `bytes`
template<frames::payload P, frames::header H>
[[nodiscard]] constexpr auto
try_construct_payload([[maybe_unused]] header<H> const hdr, std::span<std::byte const> const bytes)
    -> std::optional<payload_view_t<P>> //
{
  using view = payload_view_t<P>;
  if constexpr (frames::is_any<view>) {
    static_assert(
        header<H>::has_id,
        "an any payload selects its candidate by the header's rbe::id field, and this header has none"
    );
    return view::make(hdr.id(), bytes); // any
  }
  else if constexpr (frames::is_many<view>) {
    return view::make(bytes, detail::payload_count(hdr)); // many
  }
  else if constexpr (detail::has_make<view>) {
    return view::make(bytes); // proxy, a nested frame
  }
  else {
    return std::optional {view {bytes}}; // an opaque span-constructible payload, e.g. blob
  }
}

} // namespace rbe::frames::dsrl
