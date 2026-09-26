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
#include <rbe/annotations/empty.hpp>
#include <rbe/core/overload_set.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/dsrl/return_type.hpp>
#include <rbe/dsrl/tags.hpp>
#include <rbe/framing/detail/base_tags.hpp>
#include <rbe/framing/detail/candidate_list.hpp>
#include <rbe/framing/dsrl/concepts.hpp>
#include <rbe/framing/dsrl/detail/any_dispatcher.hpp>

// --- STD ---
#include <cassert>
#include <cstddef>
#include <optional>
#include <span>
#include <utility>

namespace rbe::dsrl {

template<frame_wirable_class... Args>
class any : public rbe::detail::any_tag {
public:
  using candidates  = rbe::detail::candidate_list<Args...>;
  using buffer_type = std::span<std::byte const>;
  using size_type   = std::size_t;
  using id_type     = candidates::id_type;

  // --- Static function members ---

  /**
   * @brief Resolve how many bytes the candidate selected by `id` takes in `data`, without constructing it
   *
   * A known id answers with that candidate's wire size, which makes the any self-delimiting: a frame
   * carrying it knows where it ends and iteration can go on past it. An unknown id has no length to go by,
   * so the any spans the rest of the buffer and iteration naturally stops at it.
   *
   * @return The length of the candidate in bytes, nullopt if the buffer is too short to hold it
   */
  [[nodiscard]] static constexpr auto parse_length(id_type const id, buffer_type const data)
      -> std::optional<size_type> {
    return length_of(candidates::index_of(id), data);
  }

  [[nodiscard]] static constexpr auto trim(id_type const id, buffer_type const data) -> std::optional<buffer_type> {
    return parse_length(id, data) //
        .transform([data](size_type const length) { return data.first(length); });
  }

  /**
   * @brief Construct an any over `data` after checking everything the contract allows checking
   *
   * This is the wide-contract counterpart of the constructor: the only precondition it cannot check is the
   * one nothing can, that the bytes really are the candidate `id` names. Construction through it costs no
   * more than through the constructor, as the resolved index and the narrowed span are handed over.
   *
   * @return The any, nullopt if the candidate does not fit in the buffer
   */
  [[nodiscard]] static constexpr auto parse(id_type const id, buffer_type const data) -> std::optional<any> {
    auto const index = candidates::index_of(id);
    return length_of(index, data) //
        .transform([&](size_type const length) { return any {index, data.first(length)}; });
  }

  // --- Constructors ---

  /**
   * Any constructor takes an id and a span of bytes, and constructs an any object that can be used to dispatch
   * at runtime to the corresponding candidate.
   *
   * Any will interpet the span of bytes as the wire representation of the candidate type corresponding to the given id
   * Providing unkonw ids is allowed and will be dispatched to the fallback overload.
   *
   * @note data is truncated to the wire_size of the candidate type if the id is known,
   * otherwise it is left as-is. This way iteration naturally stops at the end of an unknown
   * candidate for those frames whose length is implicitly extracted from any rather than
   * explicitly annotated in the header.
   *
   * Preconditions are narrow contract, violating either of them is undefined behavior:
   *   - the bytes are the wire representation of the candidate the id selects, not of another one
   *   - the candidate fits in the buffer: data.size() >= *parse_length(id, data)
   *
   * Over a buffer that may not hold a whole candidate yet use parse()
   * instead, which reports the violation rather than running into it.
   */
  constexpr any(id_type const id, buffer_type const data) :
    id_(id), //
    index_(candidates::index_of(id)), //
    data_(trim_to(index_, data)) { }

  template<typename... T>
  constexpr auto match(T&&... callbacks) const -> decltype(auto) {
    return match(rbe::overload {std::forward<T>(callbacks)...});
  }

  /**
   * @brief Dispatches the `any` object to the corresponding overload based on its ID.
   *
   * Evaluates the internal ID against the available candidate types, deserializes
   * the matching candidate, and invokes the appropriate overload from the provided set.
   *
   * Overload Set Rules
   * - Unambiguous matching: Eager and lazy overloads can be freely mixed, but must
   *   remain strictly unambiguous. For any given candidate type, exactly one overload
   *   must be invocable (either with the eager type or its lazy proxy).
   *
   * - Mandatory fallback: The overload set must provide a fallback handler constrained
   *   by the `rbe::unhandled` concept.
   *
   * - Unrecognized IDs: To distinguish between known-but-unhandled IDs and completely
   *   unrecognized IDs, an optional fallback constrained by `rbe::unknown` can be provided additionally.
   *
   * - Concept grouping: Custom C++ concepts can be used to group multiple candidate types
   *   into a single overload. However, if a concept is satisfied by both the eager and lazy
   *   forms of the same type, the dispatch will be ambiguous and fail to compile.
   *
   * @tparam T The overload set type. Must satisfy the `any_dispatcher` concept for the candidates of this `any`.
   * @param overload_set The visitor or overload set to dispatch to.
   * @return The result of the invoked overload. All invocable overloads must share the same return type.
   */
  template<any_dispatcher<candidates> T>
  constexpr auto match(T&& overload_set) const -> decltype(auto) {
    using std::ranges::to;
    template for (constexpr auto candidate: detail::handled_types<T, candidates>() | to<static_array>()) {
      using candidate_type = [:candidate:];
      if (candidates::template index_of<candidate_type>() == index_) {
        return detail::dispatch_matched<candidate_type>(std::forward<T>(overload_set), data_);
      }
    }

    return detail::dispatch_unmatched(
        std::forward<T>(overload_set), //
        rbe::detail::unmatched {.id = id_, .known_id = known_id(), .data = data_}
    );
  }

  template<typename T>
    requires(not any_dispatcher<T, candidates>)
  constexpr auto match(T /**/) const -> decltype(auto) {
    detail::diagnose_any_dispatcher<T, candidates>();
  }

  template<rbe::detail::belongs_to<candidates> T>
  [[nodiscard]] constexpr auto is() const -> bool {
    return index_ == candidates::template index_of<T>();
  }

  template<rbe::detail::belongs_to<candidates> T, strategy S = lazy_t>
    requires(wirable_class<T>)
  [[nodiscard]] constexpr auto as(S strategy = lazy) const -> std::optional<return_type<S, T>> {
    return is<T>() ? std::optional<return_type<S, T>>(rbe::deserialize<T>(data_, strategy)) : std::nullopt;
  }

  template<rbe::detail::belongs_to<candidates> T, strategy S = lazy_t>
    requires(explicitly_empty<T>)
  [[nodiscard]] constexpr auto as() const -> std::optional<T> {
    return is<T>() ? std::optional<T>(T {}) : std::nullopt;
  }

  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto id() const -> id_type { return id_; }

  [[nodiscard]] constexpr auto known_id() const -> bool { return candidates::contains(index_); }

  [[nodiscard]] constexpr auto length() const -> size_type { return data_.size(); }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return data_.data(); }

private:
  using index_type = rbe::detail::candidate_index;
  /**
   * @brief Construct from an index already resolved over a span already narrowed to the candidate
   *
   * The factory path holds both, so going through the public constructor would pay a second time for the
   * id lookup and for the narrowing. `index_type` is a type of its own precisely so this overload can never
   * be selected by an id, which is an integer just as often as an index is.
   *
   * Preconditions:
   *   - index == candidates::index_of(id)
   *   - data is already narrowed: data.size() == *parse_length(id, data)
   */
  constexpr any(index_type const index, buffer_type const data) :
    id_(candidates::ids[index]), //
    index_(index), //
    data_(data) { }

  /// @return The candidate's wire size for a known id, the rest of the buffer for an unknown one
  [[nodiscard]] static constexpr auto length_of(index_type const index, buffer_type const data)
      -> std::optional<size_type> {
    if (not candidates::contains(index)) {
      return data.size();
    }
    auto const wire_size = candidates::wire_size[std::to_underlying(index)];
    return wire_size <= data.size() ? std::optional<size_type> {wire_size} : std::nullopt;
  }

  /// Narrowing counterpart of length_of(), narrow contract: data must hold the candidate whole
  [[nodiscard]] static constexpr auto trim_to(index_type const index, buffer_type const data) -> buffer_type {
    if (not candidates::contains(index)) {
      return data;
    }
    assert(candidates::wire_size[std::to_underlying(index)] <= data.size());
    return data.first(candidates::wire_size[std::to_underlying(index)]);
  }

  id_type id_;
  index_type index_;
  buffer_type data_;
};

} // namespace rbe::dsrl
