/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file find_nsdm_with.hpp
 * @date 09/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/utils.hpp>

// --- STD ---
#include <stdexcept>

namespace rbe::detail {

consteval auto annotated_nsdm(std::meta::info type, unique_annotation auto const ann) -> std::meta::info {
  auto const members               = nsdm(type);
  auto const has_unique_annotation = [ann](auto member) { return has_annotations(member, ann); };
  auto const it                    = std::ranges::find_if(members, has_unique_annotation);
  if (it == std::ranges::end(members)) {
    throw std::invalid_argument("There isn't a member annotated with such annotation");
  }
  return *it;
}

} // namespace rbe::detail
