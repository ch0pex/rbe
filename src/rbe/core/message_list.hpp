/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file message_list.hpp
 * @date 05/08/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/concepts/wirable.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---

namespace rbe {

template<typename... T>
struct type_list { };

template<wirable... T>
struct msg_list {
  using types        = type_list<T...>;
  using variant_type = std::variant<T...>;
  using tuple_type   = std::tuple<T...>;
};


} // namespace rbe
