/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_delimiting_tags.hpp
 * @date 12/10/2026
 * @brief The delimiting categories of a frame, as tag types
 */

#pragma once

// --- Includes ---

// --- STD ---
#include <concepts>

namespace rbe::frames {

namespace detail {

/// The common base of every tag, so that rbe::frames::delimiting_tag is one std::derived_from as well
struct delimiting_tag { };

} // namespace detail

/*
 * @verbatim
 *   Frame
 *   |-- buffer_delimited                      O(1)
 *   `-- self_delimited
 *       |-- explicitly_delimited
 *       |   |-- length_delimited              O(1)
 *       |   `-- count_delimited
 *       |       |-- static_stride_delimited   O(1)
 *       |       `-- !static_stride_delimited  O(n)
 *       `-- implicitly_delimited
 *           |-- static_delimited              O(1), a compile-time constant
 *           `-- (an id, an extended header)   O(1), no leaf of its own
 * @endverbatim
 */
struct buffer_delimited_tag : detail::delimiting_tag { };
struct self_delimited_tag : detail::delimiting_tag { };
struct explicitly_delimited_tag : self_delimited_tag { };
struct length_delimited_tag : explicitly_delimited_tag { };
struct count_delimited_tag : explicitly_delimited_tag { };
struct static_stride_delimited_tag : count_delimited_tag { };
struct implicitly_delimited_tag : self_delimited_tag { };
struct static_delimited_tag : implicitly_delimited_tag { };

/**
 * @brief One of the delimiting categories above
 *
 * The two roots are the exhaustive answer to "who knows where this frame ends": the frame itself
 * (self_delimited_tag and its refinements) or the buffer it was handed. A frame that resolves its own
 * length in code is not a third answer -- it is the first one, and it says so by declaring
 * self_delimited_tag, the bare root: it finds its own end, by no mechanism the hierarchy below names.
 *
 * A tag answers the mechanism and only the mechanism. What the resolution costs is the other axis
 * (rbe::frames::constant_length), which the library deduces from the chain of tags, and a frame that
 * resolves itself in code is presumed to honour the O(1) length() contract every frame signs rather than
 * being asked to declare a cost -- see detail::is_constant_length.
 */
template<typename T>
concept delimiting_tag = std::derived_from<T, detail::delimiting_tag>;

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
