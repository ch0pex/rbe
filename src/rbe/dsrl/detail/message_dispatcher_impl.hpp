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
#include <rbe/core/message_list.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/dsrl/msg.hpp>
#include <rbe/dsrl/tags.hpp>

// --- STD ---
#include <concepts>
#include <span>
#include <stdexcept>
#include <string>

namespace rbe::detail {

/// Throwing diagnostic for why `Overload` fails to be a valid `message_dispatcher` for `MsgList` --
/// pinpoints the specific requirement violated instead of leaving the caller with a generic "constraint
/// not satisfied" error. Also doubles as the concept's actual implementation (see
/// `detail::is_message_dispatcher` below), so the two never drift apart: a dispatcher is valid exactly
/// when this doesn't throw.
template<typename Overload, typename MsgList>
consteval auto diagnose_message_dispatcher() -> void {
  using id_type = MsgList::id_type;

  if (not std::invocable<Overload&, id_type> and not std::invocable<Overload&>) {
    throw std::invalid_argument(
        "overload set must contain a fallback callback (invocable with the observed id, or with no "
        "arguments at all) to handle unrecognized messages"
    );
  }

  if (std::invocable<Overload&, id_type> and std::invocable<Overload&>) {
    throw std::invalid_argument(
        "overload set is ambiguous: it has both an id-based fallback and a no-argument fallback for "
        "unrecognized messages -- keep only one"
    );
  }

  template for (constexpr auto candidate: MsgList::types) {
    using candidate_type = [:candidate:];
    if constexpr (std::invocable<Overload&, dsrl::msg<candidate_type>> and std::invocable<Overload&, candidate_type>) {
      throw std::invalid_argument(
          "overload set is ambiguous for message type '" + std::string(display_string_of(candidate)) +
          "': it is invocable with both its lazy (msg<T>) and eager (T) form -- keep only one"
      );
    }
  }
}

template<class Overload, class IdType>
auto dispatch_unmatched(Overload overload_set, IdType id) -> decltype(auto) {
  if constexpr (std::invocable<Overload&, IdType>) {
    return overload_set(id);
  }
  else {
    return overload_set();
  }
}

/// Called once `MsgType` is known to be the candidate matching the observed id -- what's left is
/// purely a compile-time overload resolution: `overload_set` may take either `msg<MsgType>` (lazy) or
/// `MsgType` itself (eager), and whichever it's invocable with is used (both can't apply at once --
/// `message_dispatcher` already rules that out), or it falls back to `dispatch_unmatched` if it's
/// invocable with neither.
template<class MsgType, class Overload, class IdType>
auto dispatch_matched(Overload overload_set, IdType id, std::span<std::byte const> buffer) -> decltype(auto) {
  if constexpr (std::invocable<Overload&, dsrl::msg<MsgType>>) {
    return overload_set(rbe::deserialize<MsgType>(buffer, dsrl::lazy));
  }
  else if constexpr (std::invocable<Overload&, MsgType>) {
    return overload_set(rbe::deserialize<MsgType>(buffer, dsrl::eager));
  }
  else {
    return dispatch_unmatched(overload_set, id);
  }
}

} // namespace rbe::detail
