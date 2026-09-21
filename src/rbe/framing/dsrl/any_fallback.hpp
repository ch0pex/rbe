/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file unknown.hpp
 * @date 21/09/2026
 * @brief Unknown and unhandled any representation
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/id.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---

// NOTE: this file is deliberately not in the dsrl namespace,
// not because it's used in both dslr and srl, but because
// it would be too verbose to write rbe::dsrl::unmatched in the match callbacks
// Moreover probably in the future it will be used in other contexts,
// so it is better to keep it in the rbe namespace
namespace rbe {

namespace detail {


template<id_like T>
struct unmatched {
  using id_type     = T;
  using buffer_type = std::span<std::byte const>;

  id_type id;
  bool known_id;
  buffer_type data;
};


} // namespace detail

/**
 * @brief A type that represents an unhandled/unknown candidate in an any match overload set.
 *
 * This must be used as a flallback overload in any match calls,
 * to handle the case where the id of the any does not match any of the known candidates.
 */
template<typename T>
concept unmatched = detail::specialization_of(^^T, ^^detail::unmatched);

} // namespace rbe
