/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file concepts.hpp
 * @date 20/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/throw_check.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/return_type.hpp>
#include <rbe/dsrl/tags.hpp>
#include <rbe/framing/dsrl/detail/any_dispatcher.hpp>
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace rbe::dsrl {

namespace detail {

using std::get; // so `get<I>(frame)` parses as a template-id and is found by ADL

/// `get<I>` the way structured bindings look for it: a member, or a free function found by ADL
template<typename T, std::size_t I>
concept has_get = requires(T const& frame) { frame.template get<I>(); } or requires(T const& frame) { get<I>(frame); };

} // namespace detail

/**
 * @brief A frame deserializer: a non-owning view over a frame's bytes that decodes it lazily
 *
 * On top of the shape, this is the API rbe::dsrl::frame offers and the one generic code (flatten, many)
 * relies on.
 *
 * @note one common mistake that the user might make
 * is that they might mix rbe::dsrl::frame with rbe::frame we could check
 * if payload is a serder to provide them a btter error message. However
 * it's not possible to include here the serder concepts because it would
 * create a circular dependency, so we will have to live with the current error message
 * until we find a better way to do it.
 */
template<typename T>
concept is_frame = rbe::is_frame<T> and requires(T const ct) {
  typename T::buffer_type;
  typename T::size_type;

  { T::make(ct.as_span()) } -> std::same_as<std::optional<T>>;
  { ct.header() } -> std::same_as<typename T::header_return_type>;
  { ct.payload() } -> std::same_as<typename T::payload_return_type>;
  { ct.length() } -> std::same_as<typename T::size_type>;
  { ct.as_span() } -> std::same_as<typename T::buffer_type>;
  { ct.data() } -> std::same_as<std::byte const*>;

  requires std::constructible_from<T, typename T::buffer_type>;
  requires frame_header<typename T::header_type>;
  requires frame_payload<typename T::payload_type>;
};


/// A frame that can be taken apart: `auto [header, payload] = frame`
template<typename T>
concept decomposable_frame = is_frame<T> and detail::has_get<T, 0> and detail::has_get<T, 1>;


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
template<typename Overload, typename CandidateList>
concept any_dispatcher = rbe::detail::no_throw(detail::diagnose_any_dispatcher<Overload, CandidateList>);

} // namespace rbe::dsrl
