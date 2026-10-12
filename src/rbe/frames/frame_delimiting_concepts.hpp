/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_delimiting_concepts.hpp
 * @date 11/09/2026
 * @brief Classification of every frame by how its length is resolved from the wire.
 *
 * The concepts below form a hierarchy: each node refines its parent, and the leaves are mutually
 * exclusive, so any given frame matches exactly one leaf. The right column is the algorithmic
 * complexity of extracting the frame length for that category.
 *
 * @verbatim
 *   Frame
 *   |-- custom_delimited                     Any complexity
 *   |-- buffer_delimited                     O(1)
 *   `-- self_delimited
 *       |-- explicitly_delimited
 *       |   |-- length_delimited             O(1)
 *       |   `-- count_delimited
 *       |       |-- static_stride_delimited  O(1)
 *       |       `-- !static_stride_delimited O(n)
 *       `-- implicitly_delimited
 *           |-- static_delimited             O(1), a compile-time constant
 *           `-- (an id, an extended header)  O(1), no leaf of its own
 * @endverbatim
 *
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/annotation_concepts.hpp>
#include <rbe/annotations/length.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/frames/detail/frame_delimiting.hpp>
#include <rbe/frames/frame_concepts.hpp>
#include <rbe/frames/frame_delimiting_tags.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>

namespace rbe::frames {

// --- The traits the concepts below are written over ---

/**
 * @brief The category the shape of a frame puts it in, which is what every library frame declares
 *
 * @tparam HeaderType  the header of the frame
 * @tparam PayloadType the payload the header describes
 */
template<header HeaderType, typename PayloadType>
using frame_delimiting_tag = [:detail::frame_delimiting<HeaderType, PayloadType>():];

/**
 * @brief The category a frame belongs to
 *
 * Every frame carries it as a member -- rbe::frames::is_frame requires it -- and a library frame has
 * frame_delimiting_tag deduce it, so this is the spelling to read it by rather than a traits lookup.
 */
template<is_frame T>
using delimiting_concept_t = typename T::delimiting_concept;


/**
 * @brief A frame whose length is resolved by user-provided logic.
 *
 * Escape hatch for delimiting schemes that none of the other categories model. Its complexity is
 * whatever the custom resolver costs, so it sits outside the O(1) / O(n) classification.
 *
 * @note Never deduced: a frame is custom-delimited only by declaring `delimiting_concept`. A frame that
 * does find its own end should declare the category it belongs to instead -- a terminator scan is
 * `self_delimited_tag` -- and keep this for the schemes that fit nowhere.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept custom_delimited = is_frame<T> and detail::is_tag<T, custom_delimited_tag>;

/**
 * @brief A frame that can find its own end without knowing the buffer size.
 *
 * The length is resolved from a header wire field, a `payload_count`, the static size of the payload
 * types, or a nested frame that is itself self-delimiting (see `rbe::frames::delimiting_concept_t`).
 * Such a frame may be read from a larger buffer, trailing bytes being either padding or the following
 * frames, which is what lets a sequence of frames share a single buffer.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept self_delimited = is_frame<T> and detail::is_tag<T, self_delimited_tag> and not custom_delimited<T>;

/**
 * @brief A frame whose payload extends to the end of the buffer it is read from.
 *
 * The buffer size is the frame length, so the buffer must cover exactly the frame: trailing bytes are
 * taken as payload, never as padding. Such a frame can only be the last one in a buffer and a sequence
 * of them cannot be split without an external length. Resolving its length is O(1) (the buffer size).
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept buffer_delimited = is_frame<T> and detail::is_tag<T, buffer_delimited_tag> and not custom_delimited<T>;

/**
 * @brief A self-delimiting frame whose length is resolved by reading a header wire field.
 *
 * The header carries an explicit annotation for the total frame length (`rbe::frame_length`), the
 * payload length (`rbe::payload_length`), or the element count (`rbe::payload_count`).
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept explicitly_delimited = self_delimited<T> and detail::is_tag<T, explicitly_delimited_tag>;

/**
 * @brief An explicitly-delimited frame whose length comes from a byte-length field.
 *
 * The header carries `rbe::frame_length` or `rbe::payload_length`, so the length is read directly as a
 * number of bytes: O(1), regardless of what the payload holds.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept length_delimited = explicitly_delimited<T> and detail::is_tag<T, length_delimited_tag>;

/**
 * @brief An explicitly-delimited frame whose length comes from an element count.
 *
 * The payload is a `many` run and the header carries `rbe::payload_count` (and no byte-length field),
 * so the length is the sum of the element lengths. Resolving it is O(1) when the element has a static
 * stride (see `static_stride_delimited`) and O(n) otherwise, since each element must be measured.
 *
 * @note Matches no type yet: a `payload_count` over a `many` is still classified as buffer-delimited
 * (fork 3b, see the TODO in `rbe::detail::frame_delimiting`).
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept count_delimited = explicitly_delimited<T> and detail::is_tag<T, count_delimited_tag>;

/**
 * @brief A self-delimiting frame whose length is resolved from its types, with no wire field.
 *
 * The header carries no length or count annotation, so the length is derived from the implicit size of
 * the underlying payload types rather than read off the wire.
 *
 * @note Even though all explicitly-delimited frames can also be resolved from their types, the wire is always
 * considered the golden source in RBE. So a frame cannot be both at the same time, this severs more as a
 * fallback for when the wire is missing a length field than as a category of its own.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept implicitly_delimited = self_delimited<T> and detail::is_tag<T, implicitly_delimited_tag>;

/**
 * @brief An implicitly-delimited frame whose length is a compile-time constant.
 *
 * Every byte of the frame is fixed by its types: a header of static size over a payload that is itself
 * statically sized, recursively through nested frames. `rbe::frames::static_length<T>` is that constant.
 *
 * A header annotated with `rbe::header_length` disqualifies the frame however static its payload is: the
 * header declares its own length on the wire and may be extended past `wire_size_of<header_type>()`, so
 * such a frame resolves in O(1) without being a compile-time constant. So does a frame whose payload is
 * an `rbe::frames::any`: the id settles the length with one lookup, at runtime. Both are implicitly
 * delimited and `constant_length` without being this leaf -- which is why `constant_length` is not the
 * union of the O(1) categories.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept static_delimited = implicitly_delimited<T> and detail::is_tag<T, static_delimited_tag>;

/**
 * @brief The length in bytes of a `static_delimited` frame.
 *
 * The category says the constant exists; this is its value, which is what a nested frame adds to its own
 * header and what a static-stride `many` multiplies by its count.
 *
 * @note The walk lives in `detail::static_frame_size`: concepts cannot recurse, and a tag answers
 * whether the frame is a constant, not which one.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
  requires static_delimited<T>
inline constexpr std::size_t static_length = detail::static_frame_size<T>();

/**
 * @brief A `many` whose elements all occupy the same compile-time-constant number of bytes.
 *
 * The stride is a property of the element, so this is a `many` concept and not a frame one: a frame that
 * delimits by a count inherits its cost from it (see `static_stride_delimited`).
 *
 * @tparam T The `many` to be evaluated.
 */
template<typename T>
concept static_stride = is_many<T> and requires { requires static_delimited<many_element_t<T>>; };

/**
 * @brief A `many` whose elements have to be measured one by one to be stepped over.
 *
 * Walking the run is O(n): each element resolves its own length before the next one can be reached.
 *
 * @tparam T The `many` to be evaluated.
 */
template<typename T>
concept dynamic_stride = is_many<T> and not static_stride<T>;

/**
 * @brief A count-delimited frame whose elements each have a compile-time-constant size.
 *
 * Because the element stride is static, the total length is `count * stride`, so a `payload_count`
 * payload that would otherwise be O(n) is resolved in O(1).
 *
 * @note Matches no type yet, for the same reason `count_delimited` does not.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept static_stride_delimited = count_delimited<T> and detail::is_tag<T, static_stride_delimited_tag>;

/**
 * @brief A frame whose length is resolved in O(1), the cost axis of the classification.
 *
 * Not a category and not the union of the O(1) categories: the cost belongs to the whole chain of
 * nested frames, while a tag only speaks for the frame that carries it. The walk is in
 * `detail::is_constant_length`, and only two categories are not O(1) -- a counted run with no
 * static stride, and a custom resolver.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept constant_length = is_frame<T> and detail::is_constant_length<T>();

} // namespace rbe::frames
