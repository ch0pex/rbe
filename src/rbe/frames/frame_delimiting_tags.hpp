/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_delimiting_tags.hpp
 * @date 12/10/2026
 * @brief The delimiting categories of a frame, as tag types
 *
 * The vocabulary of the classification, and nothing else: the tags, what counts as one, and what it
 * means for a frame to declare its own. How a frame that declares nothing gets one is the deduction in
 * detail/frame_delimiting.hpp, and the questions asked of the answer are the concepts in
 * frame_delimiting_concepts.hpp.
 *
 * The tags mirror that hierarchy of concepts by deriving from one another, so a category refines its
 * parent the way std::forward_iterator_tag refines std::input_iterator_tag: every concept over them is
 * one std::derived_from, and an interior node is a usable answer on its own -- a frame with an extended
 * header is implicitly delimited and not static_delimited, its only leaf.
 *
 * Which tag a frame carries follows the same rule the standard library follows for iterators: it is
 * deduced from what is structurally detectable, and declared where it is not. Every frame carries it as a
 * `delimiting_concept` member -- rbe::frames::is_frame requires one -- and the default frames, rbe::frames::frame and
 * rbe::frames::dsrl::frame, have frame_delimiting_tag deduce theirs from their shape.
 *
 * A frame that resolves its own length in code (a terminator scan, a length buried in the payload, a
 * composite discriminant) is the case the shape cannot see, and usually the case the shape gets wrong:
 * rbe::frames::dsrl::base_frame deliberately declares no tag, so such a frame has nothing to inherit and has to
 * state its category, exactly as it has to supply its own make() and length().
 */

#pragma once

// --- Includes ---

// --- STD ---
#include <concepts>

namespace rbe::detail {

/// The common base of every tag, so that rbe::frames::delimiting_tag is one std::derived_from as well
struct delimiting_tag { };

} // namespace rbe::detail

namespace rbe::frames {

/*
 * @verbatim
 *   Frame
 *   |-- custom_delimited                      Any complexity
 *   |-- buffer_delimited                      O(1)
 *   `-- self_delimited
 *       |-- explicitly_delimited
 *       |   |-- length_delimited        O(1)
 *       |   `-- count_delimited
 *       |       |-- static_stride_delimited   O(1)
 *       |       `-- !static_stride_delimited  O(n)
 *       `-- implicitly_delimited
 *           |-- static_delimited              O(1), a compile-time constant
 *           `-- (an id, an extended header)   O(1), no leaf of its own
 * @endverbatim
 */
struct buffer_delimited_tag : rbe::detail::delimiting_tag { };
struct custom_delimited_tag : rbe::detail::delimiting_tag { };
struct self_delimited_tag : rbe::detail::delimiting_tag { };
struct explicitly_delimited_tag : self_delimited_tag { };
struct length_delimited_tag : explicitly_delimited_tag { };
struct count_delimited_tag : explicitly_delimited_tag { };
struct static_stride_delimited_tag : count_delimited_tag { };
struct implicitly_delimited_tag : self_delimited_tag { };
struct static_delimited_tag : implicitly_delimited_tag { };

/**
 * @brief One of the delimiting categories above
 *
 * The three roots are the exhaustive answer to "who knows where this frame ends": the frame itself
 * (self_delimited_tag and its refinements), the buffer, or code the frame supplies.
 */
template<typename T>
concept delimiting_tag = std::derived_from<T, rbe::detail::delimiting_tag>;

/**
 * @brief A type that states which delimiting category it belongs to
 *
 * This is the part of the frame shape that rbe::frames::is_frame and rbe::frames::dsrl::is_frame check, which is why it
 * lives here and not with the concepts: the nested `requires` keeps a type without the member -- or with
 * a member that is not a tag -- a non-match rather than a hard error.
 */
template<typename T>
concept declares_delimiting_concept = requires { requires delimiting_tag<typename T::delimiting_concept>; };

} // namespace rbe::frames
