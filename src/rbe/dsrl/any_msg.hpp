/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file any_msg.hpp
 * @date 21/08/2026
 * @brief Proxy over a buffer whose concrete message type isn't known until its id is read
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/metadata.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/message_list.hpp>
#include <rbe/core/metadata_layout.hpp>
#include <rbe/core/overload.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/dsrl/detail/annotated_field.hpp>
#include <rbe/dsrl/detail/message_dispatcher_impl.hpp>
#include <rbe/dsrl/message_dispatcher_concept.hpp>
#include <rbe/dsrl/msg.hpp>
#include <rbe/dsrl/return_type.hpp>
#include <rbe/dsrl/tags.hpp>

// --- STD ---
#include <algorithm>
#include <concepts>
#include <optional>
#include <span>
#include <utility>
#include <variant>

namespace rbe::dsrl {

namespace detail {

template<is_msg_list T>
class base_any_msg {
public:
  // --- Type traits ---
  using msg_list           = T;
  using id_type            = msg_list::id_type;
  using length_type        = std::size_t;
  using variant_type       = msg_list::variant_type;
  using proxy_variant_type = msg_list::proxy_variant_type;
  using lazy_variant_type  = msg_list::proxy_variant_type;
  using buffer_type        = std::span<std::byte const>;
  using first_type         = typename[:T::types[0]:];

  // --- Constructors ---

  /**
   * @brief Constructs the proxy over a buffer without deserializing anything yet
   * @param data Raw buffer holding the wire-encoded message
   */
  constexpr explicit base_any_msg(std::span<std::byte const> const data) : data_(data) { }

  /**
   * @brief Reads the message id straight from the buffer
   * @return The id of the message currently held by the buffer
   */
  [[nodiscard]] constexpr auto id() const -> id_type { return rbe::detail::read_id_field<first_type>(data_); }

  /**
   * @brief Convenience overload that builds an `overload` set from separate callbacks and dispatches
   * through the `Overload` overload below
   * @tparam Callbacks Types of the individual callbacks
   * @param callbacks Individual callbacks combined into a single overload set
   * @return Result of invoking the matched overload
   */
  template<typename... Callbacks>
  constexpr auto match(this auto const& self, Callbacks&&... callbacks) -> decltype(auto) {
    return self.match(overload {std::forward<Callbacks>(callbacks)...});
  }

  /**
   * @brief Fallback overload picked when `Overload` doesn't satisfy `message_dispatcher`, so the
   * mismatch is reported with a readable diagnostic instead of a raw overload-resolution failure
   * @tparam Overload Callable type that failed to satisfy message_dispatcher<Overload, msg_list>
   */
  template<typename Overload>
  constexpr decltype(auto) match(Overload /**/) const {
    rbe::detail::diagnose_message_dispatcher<Overload, msg_list>();
  }

  /**
   * @brief Checks whether the buffer's observed id matches message type U
   * @tparam U Wirable message type present in msg_list::types
   * @return `true` if the buffer's id matches U's declared id, `false` otherwise
   */
  template<belongs_to<msg_list> U>
  [[nodiscard]] constexpr auto is() const -> bool {
    return default_annotation_value<rbe::id, U>() == id();
  }

  /**
   * @brief Attempts to deserialize the buffer as message type U
   * @tparam U Wirable message type to attempt deserialization as
   * @tparam S Deserialization strategy type
   * @param deser_strategy Strategy used for deserialization (same strategies/return shapes as the free
   * `rbe::deserialize`)
   * @return `U` deserialized per `deser_strategy`, or `std::nullopt` if `U` isn't what the buffer's id
   * says it is
   */
  template<wirable U, strategy S>
  [[nodiscard]] constexpr auto as(S dsrl_strategy) const -> std::optional<return_type<S, U>> {
    return is<U>() ? std::optional<return_type<S, U>> {rbe::deserialize<U>(data_, dsrl_strategy)}
                   : std::optional<return_type<S, U>> {std::nullopt};
  }

  /**
   * @brief Whether the buffer's declared `length()` exceeds the bytes actually available
   *
   * A `length` field is only as trustworthy as the bytes it came from -- corruption, a truncated
   * capture, or a wire message this build doesn't know about can all make it lie. Checking this before
   * trusting `length()` (or the clamping already done by `as_span`/`remainder`) is how callers tell
   * "this message is short" apart from "this message is empty".
   *
   * @return `true` if `length()` exceeds the buffer's size, `false` otherwise
   */
  [[nodiscard]] constexpr auto truncated(this auto const& self) -> bool { return self.length() > self.data_.size(); }

  /**
   * @brief Exposes the underlying buffer trimmed to this message's own length
   * @return This message's bytes, sized to `length()`, clamped to the buffer's actual size if
   * `length()` claims more than is available (see `truncated()`)
   */
  [[nodiscard]] constexpr auto as_span(this auto const& self) -> std::span<std::byte const> {
    return self.data_.first(std::min(self.length(), self.data_.size()));
  }

  /**
   * @brief Exposes the underlying buffer as-is, without trimming it to this message's own length
   * @return The raw buffer this proxy was constructed with, which may extend past this message
   */
  [[nodiscard]] constexpr auto data() const -> buffer_type { return data_; }

  /**
   * @brief Exposes the bytes left over past this message's own length
   * @return The buffer's bytes past `length()`, or an empty span if the buffer holds none or
   * `length()` claims more than is available (see `truncated()`)
   */
  [[nodiscard]] constexpr auto remainder(this auto const& self) -> std::span<std::byte const> {
    return self.data_.subspan(std::min(self.length(), self.data_.size()));
  }

protected:
  constexpr auto resolve() -> std::optional<proxy_variant_type> {
    template for (std::size_t index = 0; constexpr auto candidate: T::types) {
      if (msg_list::ids[index++] == id()) {
        return std::optional<proxy_variant_type> {std::in_place_type<msg<typename[:candidate:]>>, data_};
      }
    }
    return std::optional<proxy_variant_type> {std::nullopt};
  }

private:
  buffer_type data_;
};

} // namespace detail

template<is_msg_list T>
class any_msg;

/**
 * @brief Proxy class that holds any message specified in the list
 * @tparam T Message list type satisfying is_msg_list
 *
 * All the messages in the list must be identificable.
 */
template<explicit_length T>
class any_msg<T> : public detail::base_any_msg<T> {
  using base = detail::base_any_msg<T>;

public:
  // --- Type traits ---
  using typename base::first_type;
  using typename base::lazy_variant_type;
  using typename base::length_type;
  using typename base::msg_list;

  // --- Constructors ---

  using base::base;

  /**
   * @brief Reads the message length straight from the buffer
   * @return The length of the message currently held by the buffer
   */
  [[nodiscard]] constexpr auto length() const -> length_type {
    return rbe::detail::read_annotated_field<rbe::length, first_type>(this->data());
  }

  /**
   * @brief Dispatches to the overload for the candidate matching the buffer's observed id
   *
   * For the matching candidate, `overload_set` may take either `msg<T>` (lazy) or `T` itself (eager) --
   * whichever it's invocable with is used. It must also be invocable with the id, or with no arguments
   * at all, as the fallback for an unrecognized message.
   *
   * @tparam Overload Callable type satisfying message_dispatcher<Overload, msg_list>
   * @param overload_set Overload set dispatched to based on the observed message id
   * @return Result of invoking the matched overload
   */
  template<message_dispatcher<msg_list> Overload>
  constexpr auto match(Overload overload_set) const -> decltype(auto) {
    auto const observed_id = this->id();

    template for (std::size_t index = 0; constexpr auto candidate: T::types) {
      using candidate_type = [:candidate:];
      if (msg_list::ids[index++] == observed_id) {
        // Known candidate: resolve lazy vs eager (or the fallback) for candidate_type.
        return rbe::detail::dispatch_matched<candidate_type>(overload_set, observed_id, this->data());
      }
    }

    // observed_id matched none of T's candidates: dispatch to the fallback overload.
    return rbe::detail::dispatch_unmatched(overload_set, observed_id);
  }

  /**
   * @brief Determines which candidate in `T` the buffer actually holds
   * @return The matching candidate wrapped in `proxy_variant_type`, or `std::nullopt` if none of them
   * declares the observed `id()` as its own
   */
  [[nodiscard]] auto as_variant() const -> std::optional<lazy_variant_type> { return this->resolve(); }
};

template<implicit_length T>
class any_msg<T> : public detail::base_any_msg<T> {
  using base = detail::base_any_msg<T>;

public:
  // --- Type traits ---
  using typename base::buffer_type;
  using typename base::lazy_variant_type;
  using typename base::length_type;
  using typename base::msg_list;
  using typename base::proxy_variant_type;

  // --- Constructors ---

  constexpr explicit any_msg(buffer_type const buffer) : base(buffer), resolved_(this->resolve()) { }

  /**
   * @brief Reads the message length straight from the buffer
   * @return The length of the message currently held by the buffer
   */
  [[nodiscard]] constexpr auto length() const -> length_type {
    return resolved_.has_value() ? std::visit([](auto const& msg) { return msg.length(); }, *resolved_) : 0;
  }

  /**
   * @brief Dispatches to the overload for the candidate matching the buffer's observed id
   *
   * For the matching candidate, `overload_set` may take either `msg<T>` (lazy) or `T` itself (eager) --
   * whichever it's invocable with is used. It must also be invocable with the id, or with no arguments
   * at all, as the fallback for an unrecognized message.
   *
   * @tparam Overload Callable type satisfying message_dispatcher<Overload, msg_list>
   * @param overload_set Overload set dispatched to based on the observed message id
   * @return Result of invoking the matched overload
   */
  template<message_dispatcher<msg_list> Overload>
  constexpr auto match(Overload overload_set) const -> decltype(auto) {
    auto const observed_id = this->id();
    auto const visitor     = [&](auto const& msg) {
      using msg_type = typename std::remove_cvref_t<decltype(msg)>::value_type;
      return rbe::detail::dispatch_matched<msg_type>(overload_set, observed_id, msg.data());
    };

    return resolved_.has_value() ? std::visit(visitor, *resolved_)
                                 : rbe::detail::dispatch_unmatched(overload_set, observed_id);
  }

  /**
   * @brief Determines which candidate in `T` the buffer actually holds
   * @return The matching candidate wrapped in `proxy_variant_type`, or `std::nullopt` if none of them
   * declares the observed `id()` as its own
   */
  [[nodiscard]] auto as_variant() const -> std::optional<lazy_variant_type> { return resolved_; }

private:
  std::optional<proxy_variant_type> resolved_;
};

} // namespace rbe::dsrl
