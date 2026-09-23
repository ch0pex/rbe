/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file message_dispatcher_impl.hpp
 * @date 05/09/2026
 * @brief Diagnostics and dispatch helpers backing the message_dispatcher concept
 */

#pragma once

// --- Includes ---
#include <ranges>
#include <rbe/annotations/empty.hpp>
#include <rbe/core/detail/invoke_concept.hpp>
#include <rbe/core/overload_set.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/dsrl/proxy.hpp>
#include <rbe/dsrl/tags.hpp>
#include <rbe/framing/dsrl/any_unmatched.hpp>

// --- STD ---
#include <concepts>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include "rbe/core/detail/introspection.hpp"

namespace rbe::dsrl::detail {

// dsrl::proxy<T> requires wirable T, which no empty type ever satisfies can_substitute
// so it needs to be checked first
// clang-format off
consteval auto is_proxy_invocable(std::meta::info candidate, std::meta::info overload) -> bool {
  return can_substitute(^^dsrl::proxy, {candidate})
     and rbe::detail::invoke_concept(^^std::invocable, {overload, substitute(^^dsrl::proxy, {candidate})});
}

consteval auto is_match_invocable(std::meta::info candidate, std::meta::info overload) -> bool {
  overload  = add_lvalue_reference(overload);
  candidate = rbe::detail::normalize_type(candidate);
  return is_proxy_invocable(candidate, overload)
      or rbe::detail::invoke_concept(^^std::invocable, {overload, candidate});
}

consteval auto is_ambiguous_match(std::meta::info candidate, std::meta::info overload) -> bool {
  overload  = add_lvalue_reference(overload);
  candidate = rbe::detail::normalize_type(candidate);
  return is_proxy_invocable(candidate, overload)
     and rbe::detail::invoke_concept(^^std::invocable, {overload, candidate});
}

consteval auto is_bad_empty_match(std::meta::info candidate, std::meta::info overload) {
  overload  = add_lvalue_reference(overload);
  candidate = rbe::detail::normalize_type(candidate);
  return rbe::detail::invoke_concept(^^rbe::explicitly_empty, {candidate})
     and is_proxy_invocable(candidate, overload)
     and not rbe::detail::invoke_concept(^^std::invocable, {overload, candidate});
}

// clang-format on

consteval auto normalize_overload_type(std::meta::info const type) {
  if (rbe::detail::specialization_of(type, ^^dsrl::proxy)) {
    return rbe::detail::normalize_type(rbe::detail::member_alias_of(type, "value_type"));
  }
  return type;
}

/// The parameter type of `closure`'s call operator, or nullopt if it is a generic (templated) call
/// operator -- e.g. `[](rbe::unmatched auto) {}` matches by concept rather than by a single fixed type,
/// so it has nothing we can check against `CandidateList::types`
consteval auto overload_parameter_type_of(std::meta::info const closure) -> std::optional<std::meta::info> {
  for (auto const member: members_of(closure, rbe::detail::default_context)) {
    if (is_function(member) and not is_function_template(member) and is_operator_function(member) and
        operator_of(member) == std::meta::op_parentheses) {
      if (auto const params = parameters_of(member); params.size() == 1) {
        return type_of(params[0]);
      }
    }
  }
  return std::nullopt;
}

/// The closure types composing `overload` -- its own template arguments if it is a `rbe::overload<Args...>`,
/// or `overload` itself for a lone callback
consteval auto closures_of(std::meta::info const overload) -> std::vector<std::meta::info> {
  if (rbe::detail::specialization_of(overload, ^^rbe::overload)) {
    return template_arguments_of(overload) | std::ranges::to<std::vector>();
  }
  return {overload};
}

template<class Overload, class CandidateList>
consteval auto handled_types() -> std::vector<std::meta::info> {
  return CandidateList::types // all candidates
         | std::views::filter(std::bind_back(is_match_invocable, ^^Overload)) // we filter in matches
         | std::ranges::to<std::vector>(); // to vector
}

/// Throwing diagnostic for why `Overload` fails to be a valid `any_dispatcher` for `CandidateList`
template<typename Overload, typename CandidateList>
consteval auto diagnose_any_dispatcher() -> void {
  using id_type = CandidateList::id_type;

  // every overload set must be able to handle ids outside of `CandidateList`
  if (not std::invocable<Overload&, rbe::detail::unmatched<id_type>>) {
    throw std::invalid_argument(
        "overload set must contain a fallback callback (invocable with the observed id, or with no "
        "arguments at all) to handle unrecognized messages"
    );
  }

  // every non-generic callback must target one of the candidates (or the unmatched fallback) --
  // a callback for a type outside of CandidateList is otherwise silently dead code
  auto overload_types = handled_types<Overload, CandidateList>();
  for (auto const closure: closures_of(^^Overload)) {
    auto const param_type = overload_parameter_type_of(closure);
    if (not param_type) {
      continue; // generic call operator -- nothing fixed to check
    }
    auto const message_type = normalize_overload_type(rbe::detail::normalize_type(*param_type));
    // clang-format off
    if (not rbe::detail::invoke_concept(^^rbe::unmatched, {message_type})
        and not std::ranges::contains(CandidateList::types, message_type)) {
      //clang-format on
      throw std::invalid_argument(
          "overload set contains a callback for '" + std::string(display_string_of(message_type)) +
          "', which is not one of the candidate message types. Check for a typo, or a stale overload "
          "left over after removing it from the candidate list"
      );
    }
  }

  // explicitly empty candidates carry no data to defer, so they must be handled eagerly (T), never
  // through proxy<T>
  auto is_bad_empty_candidate = std::bind_back(is_bad_empty_match, ^^Overload);
  for (auto candidate: overload_types | std::views::filter(is_bad_empty_candidate)) {
    throw std::invalid_argument(
        "message type '" + std::string(display_string_of(candidate)) +
        "' is annotated rbe::empty and is handled through proxy<T> -- an empty type carries no data to "
        "defer, so it must be handled by its eager form (T) instead: replace the proxy<" +
        std::string(display_string_of(candidate)) + "> overload with one taking '" +
        std::string(display_string_of(candidate)) + "' by value"
    );
  }

  // a candidate must be handled by exactly one of its two forms -- invocable with both proxy<T> (lazy)
  // and T (eager) leaves it unclear which one dispatch_matched should pick
  auto is_ambiguous_candidate = std::bind_back(is_ambiguous_match, ^^Overload);
  for (auto candidate: CandidateList::types | std::views::filter(is_ambiguous_candidate)) {
    throw std::invalid_argument(
        "overload set is ambiguous for message type '" + std::string(display_string_of(candidate)) +
        "': it is invocable with both its lazy (proxy<T>) and eager (T) form -- keep only one"
    );
  }
}

template<class Overload, rbe::unmatched T>
constexpr auto dispatch_unmatched(Overload&& overload_set, T&& unmatched) -> decltype(auto) {
  return std::invoke(std::forward<Overload>(overload_set), std::forward<T>(unmatched));
}

/// Called once `CandidateType` is known to be the candidate matching the observed id -- what's left is
/// purely a compile-time overload resolution: `overload_set` may take either `proxy<CandidateType>` (lazy) or
/// `CandidateType` itself (eager), and whichever it's invocable with is used (both can't apply at once --
/// `message_dispatcher` already rules that out), or it falls back to `dispatch_unmatched` if it's
/// invocable with neither.
template<class CandidateType, class Overload>
constexpr auto dispatch_matched(Overload overload_set, std::span<std::byte const> buffer) -> decltype(auto) {
  // NOTE: explicitly_empty needs to be first otherwise dsrl::proxy<CandidateType>  would fail the compilation
  if constexpr (explicitly_empty<CandidateType>) {
    return std::invoke(std::forward<Overload>(overload_set), CandidateType {});
  }
  else if constexpr (std::invocable<Overload&, dsrl::proxy<CandidateType>>) {
    return std::invoke(std::forward<Overload>(overload_set), rbe::deserialize<CandidateType>(buffer, dsrl::lazy));
  }
  else if constexpr (std::invocable<Overload&, CandidateType>) {
    return std::invoke(std::forward<Overload>(overload_set), rbe::deserialize<CandidateType>(buffer, dsrl::eager));
  }
}


} // namespace rbe::dsrl::detail
