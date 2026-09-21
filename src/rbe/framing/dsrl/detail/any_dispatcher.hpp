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
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/dsrl/proxy.hpp>
#include <rbe/dsrl/tags.hpp>
#include <rbe/framing/dsrl/any_unmatched.hpp>

// --- STD ---
#include <concepts>
#include <span>
#include <stdexcept>
#include <string>

namespace rbe::dsrl::detail {

/// Throwing diagnostic for why `Overload` fails to be a valid `any_dispatcher` for `CandidateList`
template<typename Overload, typename CandidateList>
consteval auto diagnose_any_dispatcher() -> void {
  using id_type = CandidateList::id_type;

  if (not std::invocable<Overload&, rbe::detail::unmatched<id_type>>) {
    throw std::invalid_argument(
        "overload set must contain a fallback callback (invocable with the observed id, or with no "
        "arguments at all) to handle unrecognized messages"
    );
  }

  template for (constexpr auto candidate: CandidateList::types) {
    using candidate_type = [:candidate:];
    if constexpr (std::invocable<Overload&, dsrl::proxy<candidate_type>> and
                  std::invocable<Overload&, candidate_type>) {
      throw std::invalid_argument(
          "overload set is ambiguous for message type '" + std::string(display_string_of(candidate)) +
          "': it is invocable with both its lazy (proxy<T>) and eager (T) form -- keep only one"
      );
    }
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
template<class CandidateType, class Overload, class IdType>
constexpr auto dispatch_matched(Overload overload_set, IdType id, std::span<std::byte const> buffer) -> decltype(auto) {
  if constexpr (std::invocable<Overload&, dsrl::proxy<CandidateType>>) {
    return std::invoke(std::forward<Overload>(overload_set), rbe::deserialize<CandidateType>(buffer, dsrl::lazy));
  }
  else if constexpr (std::invocable<Overload&, CandidateType>) {
    return std::invoke(std::forward<Overload>(overload_set), rbe::deserialize<CandidateType>(buffer, dsrl::eager));
  }
  else {
    return dispatch_unhandled(std::forward<Overload>(overload_set), id);
  }
}

template<class CandidateType, class Overload>
consteval auto is_match_invocable() -> bool {
  return std::invocable<Overload&, dsrl::proxy<CandidateType>> or std::invocable<Overload&, CandidateType>;
}

template<class Overload, class CandidateList>
consteval auto handled_types() -> std::vector<std::meta::info> {
  std::vector<std::meta::info> types;
  template for (constexpr auto candidate: CandidateList::types) {
    using candidate_type = [:candidate:];
    if constexpr (is_match_invocable<candidate_type, Overload>()) {
      types.push_back(candidate);
    }
  }
  return types;
}

} // namespace rbe::dsrl::detail
