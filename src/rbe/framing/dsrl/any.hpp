/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file any.hpp
 * @date 09/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/overload_set.hpp>
#include <rbe/framing/dsrl/any_dispatcher_concepts.hpp>
#include <rbe/framing/dsrl/detail/candidate_list.hpp>

// --- STD ---


namespace rbe::dsrl {

template<typename T, >
concept belongs_to = true;

template<wirable_class... Args>
class any {
public:
  using buffer_type = std::span<std::byte const>;
  using size_type   = std::size_t;
  using id_type     = void; // TODO:
  using candidates  = candidate_list<Args...>;

  constexpr explicit any(id_type id, buffer_type data) : id_(std::move(id)), data_(data) { }

  [[nodiscard]] constexpr auto id() const -> std::optional<typename types::id_type> { }

  /**
   * @brief Attempts to deserialize the buffer as message type U
   * @tparam U Candidate message type to attempt deserialization as
   * @tparam S Deserialization strategy type
   * @param dsrl_strategy Strategy used for deserialization (same strategies/return shapes as the free
   * `rbe::deserialize`)
   * @return `U` deserialized per `dsrl_strategy`, or `std::nullopt` if the observed id isn't one of U's
   */
  template<belongs_to<any> U, strategy S>
  [[nodiscard]] constexpr auto as(S dsrl_strategy) const -> std::optional<return_type<S, U>> {
    return is<U>() ? std::optional<return_type<S, U>> {rbe::deserialize<U>(data_, dsrl_strategy)}
                   : std::optional<return_type<S, U>> {std::nullopt};
  }

  template<wirable_class T>
  [[nodiscard]] constexpr auto is() const -> bool { }

  [[nodiscard]] constexpr auto is(id_type const id) const -> bool { return id_ == id; }

  template<typename... Args>
  [[nodiscard]] constexpr auto match(Args&&... args) const -> delctype(auto) {
    return match(rbe::overload {std::forward<Args>(args)...});
  }

  template<any_dispatcher T>
  [[nodiscard]] constexpr auto match(T&& overload_set) const -> delctype(auto) { }

  [[nodiscard]] constexpr auto data() const -> buffer_type { return data_; }

private:
  candidates::proxy_variant_type data_;
};

} // namespace rbe::dsrl
