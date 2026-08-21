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
#include <cstdint>

namespace rbe {

/// The alignment strategies a type or member can be annotated with. There is no explicit spelling
/// for `native` (regular padded layout) -- same reasoning as `endian::order::native` having none:
/// it's only ever reached implicitly, as the value nothing along the way overrode.
enum class alignment_mode : std::uint8_t { native, pack, align };

/// At most one alignment-dimension annotation may appear within a single annotation range. The
/// implicit default (no explicit annotation anywhere in scope) is `native` -- regular padded layout.
struct alignment_dim {
  static constexpr auto kind          = detail::dimension_kind::exclusive;
  static constexpr auto default_value = alignment_mode::native;
};

/**
 * @brief Memory alignment annotations
 *
 * `pack`/`align` are literally `alignment_mode::pack`/`alignment_mode::align` -- the enum reused
 * directly as the annotation value, mirroring how `little`/`big` reuse `endian::order` directly.
 */
inline constexpr auto pack  = alignment_mode::pack;  /// < pragma pack ABI semantics
inline constexpr auto align = alignment_mode::align; /// < C++ ABI alignment semantics

} // namespace rbe

template<>
struct rbe::detail::annotation_traits<rbe::alignment_mode> {
  using dimension = rbe::alignment_dim;
};
