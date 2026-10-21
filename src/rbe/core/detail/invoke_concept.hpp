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
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <initializer_list>
#include <meta>
#include <optional>
#include <utility>

namespace rbe::detail {

template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
consteval auto invoke_concept(std::meta::info const concept_rfl, R&& arguments) -> bool {
  if (not is_concept(concept_rfl)) {
    throw std::meta::exception("reflection is not a concept", ^^invoke_concept);
  }
  return extract<bool>(substitute(concept_rfl, std::forward<R>(arguments)));
}

template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
consteval auto invoke_trait(std::meta::info const trait_rfl, R&& arguments) -> bool {
  if (not is_template(trait_rfl)) {
    throw std::meta::exception("reflection is not a template", ^^invoke_trait);
  }

  // standard traits inherit `value` from std::integral_constant, so look through the bases too
  auto const find_value = [](this auto const& self, std::meta::info const type) -> std::optional<std::meta::info> {
    if (auto const value = static_data_member(type, "value")) {
      return value;
    }
    for (auto const base: std::meta::bases_of(type, default_context)) {
      if (auto const value = self(std::meta::type_of(base))) {
        return value;
      }
    }
    return std::nullopt;
  };

  auto const trait_inst = substitute(trait_rfl, std::forward<R>(arguments));
  auto const value_rfl  = find_value(trait_inst);
  if (not value_rfl) {
    throw std::meta::exception("trait has no static data member named value", ^^invoke_trait);
  }
  return extract<bool>(*value_rfl);
}

} // namespace rbe::detail
