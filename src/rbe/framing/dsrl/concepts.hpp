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

namespace rbe::dsrl {

/**
 * @brief A frame deserializer: a non-owning view over a frame's bytes that decodes it lazily
 *
 * On top of the shape, this is the API rbe::dsrl::frame offers and the one generic code (flatten, many)
 * relies on.
 */
template<typename T>
concept is_frame = rbe::is_frame<T> and requires(T const ct) {
  typename T::buffer_type;
  typename T::size_type;

  { ct.header() } -> std::same_as<return_type<lazy_t, typename T::header_type>>;
  { ct.header(lazy) } -> std::same_as<return_type<lazy_t, typename T::header_type>>;
  { ct.header(eager) } -> std::same_as<return_type<eager_t, typename T::header_type>>;
  { ct.header(in_place) } -> std::same_as<return_type<in_place_t, typename T::header_type>>;
  // TOOO: payload getter

  { ct.header_span() } -> std::same_as<typename T::buffer_type>;
  { ct.payload_span() } -> std::same_as<typename T::buffer_type>;
  { ct.length() } -> std::same_as<typename T::size_type>;
  { T::parse_length(ct.as_span()) } -> std::same_as<std::optional<typename T::size_type>>;
  { T::parse(ct.as_span()) } -> std::same_as<std::optional<T>>;
  { ct.header_length() } -> std::same_as<typename T::size_type>;
  { ct.payload_length() } -> std::same_as<typename T::size_type>;
  { ct.as_span() } -> std::same_as<typename T::buffer_type>;
  { ct.data() } -> std::same_as<std::byte const*>;

  requires std::constructible_from<T, typename T::buffer_type>;
  requires frame_header<typename T::header_type>;
  requires frame_payload<typename T::payload_type>;
};


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
