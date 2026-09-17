/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file bits.hpp
 * @date 18/09/2026
 * @brief Explicit bit placement annotation
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotation_info.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <cstddef>
#include <cstdint>
#include <meta>

namespace rbe {

namespace detail {

/// The widest field a bit range can address is 64 bits wide, so 63 is the highest bit there is.
inline constexpr auto max_bit_index = 63;

/// The value `rbe::bits(msb, lsb)` carries.
struct bit_range {
  std::uint8_t msb, lsb;

  // NOTE: int is used here to avoid -Wconversion warnings when passing literals
  consteval bit_range(int const msb, int const lsb) :
    msb {static_cast<std::uint8_t>(msb)}, lsb {static_cast<std::uint8_t>(lsb)} {
    if (lsb < 0 or msb > max_bit_index) {
      throw std::meta::exception("bit index out of range: a bit range lives within [0, 63]", ^^bit_range);
    }
    if (msb < lsb) {
      throw std::meta::exception("inverted bit range: msb must not be below lsb", ^^bit_range);
    }
  }

  consteval auto operator==(bit_range const&) const -> bool = default;
};

/**
 * Explicit bits semantics. It joins the endianness dimension -- `[[=rbe::little, =rbe::bits(3,0)]]`
 * is correctly rejected -- without carrying an `endian::order`, so byte-order resolution simply does
 * not see it.
 */
struct bits_tag {
  using dimension  = endianness_dim;
  using value_type = bit_range;

  /**
   * `bit_range` can only bound itself by the widest field RBE supports; here the annotated field is
   * known, so the range is checked against the bits that field actually has.
   */
  static consteval auto check(annotation_info const ann, std::meta::info const entity) -> bool {
    auto const type = normalize_type(entity);
    if (not is_integral_type(type)) {
      return false;
    }
    auto const range = ann.value<bit_range>();
    return range and static_cast<std::size_t>(range->msb) < 8U * size_of(type);
  }
};

} // namespace detail

/**
 * @brief The bit range, most significant bit first, the annotated field occupies: `rbe::bits(3, 0)`.
 *
 * Both indices are inclusive and must fit in the annotated integral field. It shares the endianness
 * dimension, so it cannot be combined with `little`/`big` on the same field.
 *
 * @note Declared and checked, but not yet consumed by layout computation.
 */
inline constexpr detail::annotation_kind<detail::bits_tag> bits {};

} // namespace rbe
