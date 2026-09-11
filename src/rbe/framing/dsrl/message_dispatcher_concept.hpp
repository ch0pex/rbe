/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file message_dispatcher_concept.hpp
 * @date 05/09/2026
 * @brief Concept for callables able to dispatch over a msg_list's candidates
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/no_throw.hpp>
#include <rbe/core/message_list.hpp>
#include <rbe/dsrl/detail/message_dispatcher_impl.hpp>

namespace rbe {

/**
 * @brief Checks whether Overload is an unambiguous dispatcher for every message in MsgList
 *
 * Satisfied when, for every message type `T` in MsgList, Overload is invocable with at most one of
 * `T` (eager) or `dsrl::msg<T>` (lazy), and Overload additionally provides exactly one fallback
 * overload for unrecognized messages: either `[](id_type id) {...}` or `[]() {...}`.
 *
 * @code
 * rbe::overload{
 *     [](FooMsg const& foo) { ... },        // eager form for FooMsg
 *     [](dsrl::msg<BarMsg> const& bar) { ... }, // lazy form for BarMsg
 *     [](MsgList::id_type id) { ... },      // fallback for unrecognized ids
 * };
 * @endcode
 *
 * @tparam Overload Candidate dispatcher, typically built with rbe::overload
 * @tparam MsgList Message list the dispatcher must be able to handle
 */
template<typename Overload, typename MsgList>
concept message_dispatcher = //
    is_msg_list<MsgList> //
    and detail::no_throw(detail::diagnose_message_dispatcher<Overload, MsgList>);

} // namespace rbe
