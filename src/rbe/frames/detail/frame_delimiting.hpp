/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_delimiting.hpp
 * @date 12/10/2026
 * @brief How a frame gets its delimiting tag, and the walks a single tag cannot answer
 *
 * The deduction reads the frame itself -- the length annotations of its header and the category of its
 * payload -- and is the single authority on the precedence between them: the wire is the golden source,
 * so a header length field outranks whatever the payload types could have answered. dsrl::frame::length()
 * resolves at runtime in that same order, and the two must be changed together.
 *
 * The walks are here for the reason the classification needs functions at all: concepts cannot recurse,
 * and a tag speaks only for the frame that carries it. The size of a nested chain of static frames, and
 * the cost of resolving one, are both properties of the whole chain.
 *
 * Everything here only looks at the shape of a frame (its header_type and payload_type), so it applies
 * equally to rbe:: vocabulary frames and to their dsrl:: / srl:: lowerings: lowering never changes the
 * category.
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/annotation_concepts.hpp>
#include <rbe/annotations/length.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/frames/frame_concepts.hpp>
#include <rbe/frames/frame_delimiting_tags.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <meta>

namespace rbe::detail {

template<frames::header H, typename P>
[[nodiscard]] consteval auto frame_delimiting() -> std::meta::info;

/**
 * @brief Whether the delimiting category of T refines Tag
 *
 * One std::derived_from is all a category test ever is, which is the point of the tags. The nested
 * requires keeps a type with no category -- a non-frame, or a frame whose header and payload do not form
 * one -- a non-match instead of a hard error.
 */
template<typename T, typename Tag>
concept is_tag = requires { requires std::derived_from<typename T::delimiting_concept, Tag>; };

/// Whether the element of a `many` has a compile-time-constant size, so the run can be strided over
template<typename P>
[[nodiscard]] consteval auto has_static_stride() -> bool {
  if constexpr (frames::is_many<P> and requires { typename P::frame_type; }) {
    return std::derived_from<typename frames::many_element_t<P>::delimiting_concept, frames::static_delimited_tag>;
  }
  else {
    return false;
  }
}

/**
 * The deduction: which category the shape of a frame puts it in.
 *
 * The branches are in priority order and the order is the decision: the wire is the golden source, so a
 * header that carries a length field is the authority however static the payload types are, and a count
 * -- a length one step removed, bounding the run instead of the bytes -- comes right after it.
 *
 * The payload categories are tested before `frames::wirable_payload`, not after: a payload view is a class with no
 * non-static data members of its own, so whether `wirable` rejects it is a property of
 * detail::is_wirable_class_type (today, of its base-class exclusion) rather than of the classification.
 * Testing the categories first makes the order mean what it says.
 *
 * A header annotated with rbe::header_length is the one case where a static payload does not make a
 * static frame: the header declares its own length on the wire and may be extended past
 * wire_size_of<header_type>(), so the frame resolves in O(1) without being a compile-time constant --
 * frames::implicitly_delimited_tag, the interior node, and not its static_delimited_tag leaf.
 */
template<frames::header H, typename P>
[[nodiscard]] consteval auto frame_delimiting() -> std::meta::info {
  constexpr auto extended_header = contains_annotation<H, rbe::header_length>;

  if constexpr (contains_annotation<H, rbe::payload_length> or contains_annotation<H, rbe::frame_length>) {
    return ^^frames::length_delimited_tag;
  }
  // lower priority than a byte length field: a count only settles the extent together with the stride.
  //
  // TODO(fork 3b): flipping `counted_runs_are_delimited` to true is the whole change, and it cannot land
  // before rbe::frame_distance (fork 2): the dynamic-stride case is O(n) and dsrl::frame::length() has no
  // way to report that. It is not a free switch either -- every counted run (aquis::packet, every
  // HeaderWithCount frame) moves from buffer-delimited to count-delimited, in this classification and in
  // the length() that reads it.
  else if constexpr (
      constexpr auto counted_runs_are_delimited = false;
      counted_runs_are_delimited and contains_annotation<H, rbe::payload_count> and frames::is_many<P>
  ) {
    return has_static_stride<P>() ? ^^frames::static_stride_delimited_tag : ^^frames::count_delimited_tag;
  }
  else if constexpr (frames::is_frame<P>) {
    // the nested frame's own category, not its shape: a declaration made deep in a chain of frames has to
    // reach the outermost one, and reading the member rather than re-deducing is what carries it out.
    using nested = typename P::delimiting_concept;

    // an outer header adds bytes to a frame, it does not tell the inner one where to end
    if constexpr (std::same_as<nested, frames::buffer_delimited_tag>) {
      return ^^frames::buffer_delimited_tag;
    }
    else if constexpr (std::same_as<nested, frames::custom_delimited_tag>) {
      return ^^frames::custom_delimited_tag;
    }
    else if constexpr (std::derived_from<nested, frames::static_delimited_tag>) {
      return extended_header ? ^^frames::implicitly_delimited_tag : ^^frames::static_delimited_tag;
    }
    else {
      // the inner frame reads a field, counts its elements, scans for a terminator or has an extended
      // header of its own: the outer frame has no field of its own, so it is implicit, and its cost is
      // whatever the chain below it costs (see is_constant_length).
      return ^^frames::implicitly_delimited_tag;
    }
  }
  else if constexpr (frames::is_any<P>) {
    // the id picks the candidate and the candidate is its own length: O(1), one lookup, but a runtime
    // value, so the interior node and not the static leaf
    return ^^frames::implicitly_delimited_tag;
  }
  else if constexpr (frames::is_many<P>) {
    return ^^frames::buffer_delimited_tag; // an unbounded run: with no count, it runs to the end of the buffer
  }
  else if constexpr (frames::wirable_payload<P>) {
    // wirable, or explicitly empty: a header-only frame, whose payload is zero bytes wide.
    // TODO: a variable-size wirable payload is frames::implicitly_delimited_tag and not frames::static_delimited_tag.
    return extended_header ? ^^frames::implicitly_delimited_tag : ^^frames::static_delimited_tag;
  }
  else {
    return ^^frames::buffer_delimited_tag; // a blob, or any other view constructible from the payload bytes
  }
}

/**
 * @brief The length in bytes of a frame whose category says it is a compile-time constant
 *
 * Concepts cannot recurse and a tag does not carry a value, so the walk down the nested frames lives
 * here. It answers with the size rather than with a bool because every caller needs the value: a nested
 * frame adds it to its own header, a static-stride `many` multiplies it by the header count, and
 * rbe::frames::static_length hands it to the user.
 *
 * The constraint is what keeps this honest about an extended header: such a frame is not
 * frames::static_delimited_tag, so it never reaches here, however static its payload is.
 */
template<frames::is_frame T>
  requires std::derived_from<typename T::delimiting_concept, frames::static_delimited_tag>
[[nodiscard]] consteval auto static_frame_size() -> std::size_t {
  using payload_type = frames::payload_t<T>;

  constexpr auto header_size = rbe::wire_size_of<frames::header_t<T>>();

  if constexpr (frames::is_frame<payload_type>) {
    return header_size + static_frame_size<payload_type>();
  }
  else {
    static_assert(
        frames::wirable_payload<payload_type>,
        "a frame declaring frames::static_delimited_tag must have a payload of a compile-time-constant size: a "
        "wirable type, an explicitly empty one, or a nested frame that is itself static"
    );
    return header_size + rbe::wire_size_of<payload_type>();
  }
}

/**
 * @brief Whether resolving the frame length is O(1), which the outermost tag alone cannot answer
 *
 * The cost is a property of the whole chain: an outer frame pays whatever its nested frame's length
 * costs, and `frames::implicitly_delimited_tag` says only that the outer frame reads no field of its own. So the
 * walk goes down the payloads, and a single level that has to measure its elements one by one settles it
 * for everything above.
 *
 * Three tags are not O(1): a counted run with no static stride (one element measured per element), a
 * custom resolver, whose cost is whatever the user wrote, and a bare frames::self_delimited_tag -- a frame that
 * declares only the root says that it finds its own end and nothing at all about what that costs, which
 * is the terminator scan. A frame that does resolve in O(1) without fitting a leaf declares
 * frames::implicitly_delimited_tag instead. Every other category reads a field, looks up an id, adds static
 * sizes or takes the buffer size.
 */
template<frames::is_frame T>
[[nodiscard]] consteval auto is_constant_length() -> bool {
  using tag = typename T::delimiting_concept;

  if constexpr (std::same_as<tag, frames::custom_delimited_tag> or std::same_as<tag, frames::self_delimited_tag>) {
    return false;
  }
  else if constexpr (
      std::derived_from<tag, frames::count_delimited_tag> and
      not std::derived_from<tag, frames::static_stride_delimited_tag>
  ) {
    return false;
  }
  else if constexpr (frames::is_frame<frames::payload_t<T>>) {
    return is_constant_length<frames::payload_t<T>>();
  }
  else {
    return true;
  }
}

} // namespace rbe::detail
