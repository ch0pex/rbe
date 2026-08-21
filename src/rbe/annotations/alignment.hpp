/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file alignment.hpp
 * @date 20/08/2026
 * @brief Memory alignment annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/dimension.hpp>

// --- STD ---
#include <type_traits>

namespace rbe {

/// Two layout annotations cannot coexist within the same annotation range.
struct alignment_dim {
  static constexpr auto kind = detail::dimension_kind::exclusive;
};

/**
 * @brief Memory alignment annotations
 */
inline constexpr struct {} pack {};  /// < pragma pack ABI semantics
inline constexpr struct {} align {}; /// < C++ ABI alignment semantics

} // namespace rbe

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::pack)>> {
  using dimension = rbe::alignment_dim;
};

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::align)>> {
  using dimension = rbe::alignment_dim;
};
