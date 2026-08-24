/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file invoke.hpp
 * @date 24/08/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---

// --- STD ---
#include <initializer_list>
#include <meta>
#include <utility>

namespace rbe::detail {

template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
consteval auto invoke_concept(std::meta::info const concept_rfl, R&& arguments) -> bool {
  if (not is_concept(concept_rfl)) {
    throw std::meta::exception("reflection is not a concept", ^^invoke_concept);
  }
  return extract<bool>(substitute(concept_rfl, std::forward<R>(arguments)));
}

} // namespace rbe::detail
