/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_candidate_list.cpp
 * @date 18/09/2026
 * @brief Compile-time checks for candidate_list
 */

// --- Includes ---
#include <rbe/annotations/id.hpp>
#include <rbe/core/detail/static_string.hpp>
#include <rbe/framing/detail/candidate_list.hpp>

namespace {

// clang-format off
struct [[=rbe::id(1)]] msg_a { int a; };
struct [[=rbe::id(2)]] msg_b { int b; };
struct [[=rbe::id(3)]] msg_c { int c; };
struct [[=rbe::id(4)]] msg_d { int d; };
struct [[=rbe::id(5)]] msg_e { int e; };
struct [[=rbe::id(6)]] msg_f { int f; };

struct custom_id { int x; int y; friend constexpr auto operator==(custom_id const&, custom_id const&) -> bool = default; };

struct [[=rbe::id(custom_id{.x = 0, .y = 1})]] msg_hb { int hb; };
struct [[=rbe::id(custom_id{.x = 1, .y = 1})]] msg_st { int st; };
struct [[=rbe::id(custom_id{.x = 0, .y = 0})]] msg_cmd { int cmd; };

// a `char const*` cannot be an id: the address of a string literal is not a permitted result of a
// constant expression, so the value cannot be reflected back out of the annotation
struct [[=rbe::id(rbe::static_string {"heartbeat"})]] msg_str { int hb; };
struct [[=rbe::id(rbe::static_string {"status"})]] msg_str2 { int st; };
struct [[=rbe::id(rbe::static_string {"command"})]] msg_str3 { int cmd; };

// same id as msg_a / msg_str: a candidate list may not say one id twice
struct [[=rbe::id(1)]] msg_a_again { int a; };
struct [[=rbe::id(rbe::static_string {"heartbeat"})]] msg_str_again { int hb; };

struct non_identifiable { int x; };

static_assert(rbe::detail::compatible_candidates<msg_a, msg_b, msg_c>);
static_assert(rbe::detail::compatible_candidates<msg_hb, msg_st, msg_cmd>);
static_assert(rbe::detail::compatible_candidates<msg_str, msg_str2, msg_str3>);
static_assert(not rbe::detail::compatible_candidates<msg_a, msg_b, msg_hb>); // different id types
static_assert(not rbe::detail::compatible_candidates<msg_a, non_identifiable>); // non-identifiable type
static_assert(not rbe::detail::compatible_candidates<msg_a>); // only one candidate
static_assert(not rbe::detail::compatible_candidates<msg_a, msg_b, msg_a_again>); // repeated id
static_assert(not rbe::detail::compatible_candidates<msg_str, msg_str2, msg_str_again>); // repeated id

using candidate_list_t = rbe::detail::candidate_list<msg_a, msg_b, msg_c>;
using candidate_list_hb = rbe::detail::candidate_list<msg_hb, msg_st, msg_cmd>;
using candidate_list_str = rbe::detail::candidate_list<msg_str, msg_str2, msg_str3>;

static_assert(candidate_list_t::ids == std::array {1, 2, 3});
static_assert(candidate_list_hb::ids == std::array {custom_id{.x = 0, .y = 1}, custom_id{.x = 1, .y = 1}, custom_id{.x = 0, .y = 0}});
static_assert(candidate_list_str::ids[0].get() == "heartbeat");
static_assert(candidate_list_str::ids[1].get() == "status");
static_assert(candidate_list_str::ids[2].get() == "command");
static_assert(std::same_as<candidate_list_t::id_type, int>);
static_assert(std::same_as<candidate_list_hb::id_type, custom_id>);
static_assert(std::same_as<candidate_list_str::id_type, rbe::static_string>);

static_assert(candidate_list_t::count == 3);
static_assert(candidate_list_hb::count == 3);
static_assert(candidate_list_str::count == 3);
static_assert(candidate_list_t::index_of(1) == 0);
static_assert(candidate_list_t::index_of(2) == 1);
static_assert(candidate_list_t::index_of(3) == 2);
static_assert(candidate_list_hb::index_of(custom_id{.x = 0, .y = 1}) == 0);
static_assert(candidate_list_hb::index_of(custom_id{.x = 1, .y = 1}) == 1);
static_assert(candidate_list_hb::index_of(custom_id{.x = 0, .y = 0}) == 2);
static_assert(candidate_list_str::index_of(rbe::static_string {"heartbeat"}) == 0);
static_assert(candidate_list_str::index_of(rbe::static_string {"status"}) == 1);
static_assert(candidate_list_str::index_of(rbe::static_string {"command"}) == 2);
static_assert(candidate_list_t::wire_size == std::array {sizeof(msg_a), sizeof(msg_b), sizeof(msg_c)});

// clang-format on

} // namespace
