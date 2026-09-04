/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_dsrl_return_type.cpp
 * @date 04/09/2026
 * @brief Static assertions for the strategy -> return type mapping in rbe::dsrl::return_type
 */

// --- Includes ---
#include "common_structs.hpp"

#include <rbe/dsrl/return_type.hpp>

// --- STD ---
#include <concepts>

namespace {

using rbe::dsrl::return_type;

// clang-format off
static_assert(std::same_as<return_type<rbe::dsrl::eager_t, Message>::type, Message>);
static_assert(std::same_as<return_type<rbe::dsrl::lazy_t, Message>::type, rbe::dsrl::msg<Message>>);
static_assert(std::same_as<return_type<rbe::dsrl::in_place_t, Message>::type, Message const&>);
static_assert(std::same_as<return_type<rbe::dsrl::in_place_mut_t, Message>::type, Message&>);
// clang-format on

} // namespace
